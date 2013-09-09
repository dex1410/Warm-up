/*
defs.h - Included typedefs for datatypes 
*/
#include <defs.h>

/*
stdarg.h - Included variable list definitions
*/
#include <stdarg.h> 

/*
io.h - Included the asm instruction (out) in order to update the cursor 
*/
#include <io.h>

#define START_MEMORY 0xB8000// Staring address of VGA BUFFER ( VGA MEMORY )

char* video_memory = (char *)START_MEMORY;
void update_cursor(void);

char c;

/*
Function to find the length of the string.
*/
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
void write_string( int color, const char *string )
{
    volatile char *video = (volatile char*)video_memory;
    //int length = strlen(string);
    while( *string != NULL )
    {
        *video++ = *string++;
        *video++ = color;
//	(char *)video_memory++;
    }
    video_memory = (char *)(video);
    update_cursor();
}


/*
Trivial function. Writes a byte ( character ) to the current pointer of the video
buffer and updates the video memory.
*/
void write_char(int color, char c)
{
	volatile char *video = (volatile char*)video_memory;
	*video++ = c;
	*video++ = color;
	video_memory = (char *)(video);
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


char *convert(unsigned int num, int base)
{
static char buf[33];
char *ptr;

ptr=&buf[sizeof(buf)-1];
*ptr='\0';
do
{
*--ptr="0123456789abcdef"[num%base];
num/=base;
}while(num!=0);
return(ptr);
}

/*
Prints the given values to the screen. Since it is 64 bit machine, the way va_arg
works is different. All the variables passed through the registers are pushed into
register-saved-area of the stack. Then the general purpose registers and the floating
point registers follow. Thus the va_arg function accesses this area smoothly.
*/
int kprintf(char *fmt,...){
	char *p;
	int i; // integer argument
	//unsigned u; // unsigned int argument
	char *s; // string argument
	va_list arg_p; // pointer to the variable argument list
	va_start(arg_p, fmt); /* Initializes the pointer to retrieve the additional
				parameters. Should call va_end before end. fmt is
				also passed because, we need to know where the last
				fixed argument is in order to find the starting of 
				the variable list. 
			     */
	for(p=fmt; *p ; p++){
		if(*p != '%'){
			write_char(0x1F,*p);
			continue;
		}
		switch(*++p){
			case 'c': i = va_arg(arg_p, int);
				  write_char(0x1F,i); 
				  break;
			case 's': s = va_arg(arg_p, char*);
				  write_string(0x1F,s);
				  break;
			case 'd': i = va_arg(arg_p, int);
				  write_string(0x1F,convert(i,10));
				  break;
		}
	} 
	va_end(arg_p);
	return 1;	
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
	int ii=9;
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
    write_string(0x1F,"sjdhkajsdhajkdhkjasdhkajsasdsadasdasdasdasdadasfgfghhjkui");
    kprintf("String is %s and Integer is %d","Chid",ii);
	while(1);
}
