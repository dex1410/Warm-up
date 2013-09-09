#include <defs.h>
#include <io.h>
#define START_MEMORY 0xB8000// Staring address of VGA BUFFER ( VGA MEMORY )

char* video_memory = (char *)START_MEMORY;
void update_cursor(void);

char c;

int strlen(const char *s){
	int length=0;
	char* local_s = (char *)s;
	while(*local_s){
		length++;
		local_s++;
	}
	return length;
}
/*
Writes to the video memory buffer. The text screen video memory for color monitors
resides at 0xB8000. Text mode memory takes two bytes for each character on the
screen. One is the ASCII code byte, other is the attribute byte. The attribute
byte carries the foreground color in the lower 4 bits, and the background color
in the higher 3 bits. 0x00 is black on black and nothing can be seen. 0x07 is
lightgrey on black. 0x1F is white on blue. 

I guess as and when we write to this buffer of the video memory, it
consumes it and puts it out on the screen. 

*/
void write_string( int colour, const char *string )
{
    volatile char *video = (volatile char*)video_memory;
    //int length = strlen(string);
    while( *string != NULL )
    {
        *video++ = *string++;
        *video++ = colour;
//	(char *)video_memory++;
    }
    video_memory = (char *)(video);
    update_cursor();
}


/*
Updates the position of the cursor. Memory is linear. So if we want to go to 
5*6 ( row*column ), multiplying 5 by 80 will give 400. Plus 6 will give 6 bytes.
Thus 406th byte is the actual position of 5*6. 
0x3d0 - 0x3DF is the port address mapping range for VGA / CGA memory. When any 
port in this port range is specified, the controller picks up the data and
gives it to the controller of the device so that the device does as we asked.
*/
void update_cursor()
{
    long diff = (long)(video_memory - START_MEMORY); 
    /* Long is used because the size a pointer in 64 bit machine is 8 bytes. 
       So we cannot type cast it to an int which is of lesser size. In C
       we can always typecast from lower size to upper size but not from
       bigger size to lower size.
    */
    int row1 = (diff/80)/2;
    int col1 = (diff%80)/2;
    unsigned short position=(row1*80) + col1;
    // cursor LOW port to vga INDEX register
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position&0xFF)); // Send the lower byte
    // cursor HIGH port to vga INDEX register
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position>>8)&0xFF)); // Send the upper byte.

    /*
	This should not be confused as sending x position first and sending y
	position second. If we need to go to 80th row and 4th column, then
	we 80*80 + 4 = 6404. In binary it is 1100100000100. This first
	outb statementwill yield 00000100 and second outb statement will yield
	00011001.
    */
}

void start(void* modulep, void* physbase, void* physfree)
{
	// kernel starts here
/*int cr0;
__asm__("mov %%cr0, %0;"
      :"=r"(cr0)
        ://No input operands
      :"%eax"
     );
*/
    c=(*(volatile unsigned short*)0x410)&0x30;
    //c can be 0x00 or 0x20 for colour, 0x30 for mono.
}

#define INITIAL_STACK_SIZE 4096
char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;
extern char kernmem, physbase;

void boot(void)
{
	// note: function changes rsp, local stack variables can't be practically used
	volatile register char *rsp asm ("rsp");
	volatile register char *temp1, *temp2;
	int length;
	int *p = NULL;
	//int q;
	loader_stack = (uint32_t*)rsp;
	rsp = &stack[INITIAL_STACK_SIZE];
	(*p)++;
	start(
		(char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase,
		&physbase,
		(void*)(uint64_t)loader_stack[4]
	);
	for(
		temp1 = "!!!!! starsdft() !!!!!", length = strlen((const char*)temp1), temp2 = (char*)video_memory;
		*temp1;
		temp1 += 1, temp2 += 2
	) *temp2 = *temp1;
	video_memory = (char *)(temp2);
	update_cursor();
    //write_string("!!!! start returned !!!!");
    write_string(0x1F,"Chidambaram");
    write_string(0x1F,"\nsjdhkajsdhajkdhkjasdhkajsasds\nadasdasdasda\nsdadasfgfghhjkui");
	while(1);
}
