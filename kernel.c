#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* --- 1. VGA & Hardware Helpers --- */
static uint16_t* const VGA_BUFFER = (uint16_t*) 0xB8000;
static size_t term_col = 0, term_row = 0;
static uint8_t term_color = 0x0F; // Default to White

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}
static inline uint8_t inb(uint16_t p) { uint8_t r; asm volatile("inb %1, %0":"=a"(r):"Nd"(p)); return r; }

void term_putc(char c) {
    if (c == '\n') { term_col = 0; term_row++; }
    else {
        VGA_BUFFER[term_row * 80 + term_col] = (uint16_t) c | (uint16_t) term_color << 8;
        if (++term_col == 80) { term_col = 0; term_row++; }
    }
}
void term_print(const char* str) { for (size_t i = 0; str[i] != '\0'; i++) term_putc(str[i]); }

/* --- 2. String Management --- */
int str_equal(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 == *(unsigned char*)s2;
}
void str_copy(char* dest, const char* src) { while ((*dest++ = *src++)); }
int str_starts(const char* str, const char* pre) {
    while (*pre) { if (*pre++ != *str++) return 0; }
    return 1;
}

/* --- 3. RAM File System --- */
typedef struct {
    char name[32];
    char content[128];
    bool active;
} file_t;
file_t disk[10];

/* --- 4. UI Helpers --- */
void print_prompt() {
    uint8_t old_color = term_color;
    
    term_color = 0x0A; // Light Green for User/Machine
    term_print("team-OS:");
    
    term_color = 0x0F; // White for the separator and prompt
    term_print(":~$ ");
    
    term_color = old_color; // Back to whatever color the user set
}

/* --- 5. Shell Command Processor --- */
void execute_command(char* input) {
    term_putc('\n');

    if (str_equal(input, "help")) {
        term_print("\nCommands: where, show, create, write, run, delete, whoami, cls, color, reboot\n");
    }
    else if (str_equal(input, "where")) {
        term_print("Current Path: /root/amit/projects/myos");
    } 
    else if (str_equal(input, "whoami")) {
        term_print("User: Amit (Kernel Admin) | Mode: x86 Protected");
    }
    else if (str_equal(input, "show")) {
        bool found = false;
        term_print("Files in RAM Disk:\n");
        for(int i = 0; i < 10; i++) {
            if(disk[i].active) {
                term_print("- "); term_print(disk[i].name); term_print("  ");
                found = true;
            }
        }
        if(!found) term_print("(No files)");
    }
    else if (str_equal(input, "cls")) {
        for (size_t i = 0; i < 80 * 25; i++) VGA_BUFFER[i] = (uint16_t) ' ' | (uint16_t) term_color << 8;
        term_row = 0; term_col = 0;
        print_prompt(); // Direct return needs a prompt
        return;
    }
    else if (str_equal(input, "reboot")) {
        term_print("Rebooting...");
        outb(0x64, 0xFE);
    }
    else if (str_starts(input, "color ")) {
        char* color_name = input + 6;
        if (str_equal(color_name, "blue")) term_color = 0x01;
        else if (str_equal(color_name, "green")) term_color = 0x02;
        else if (str_equal(color_name, "red")) term_color = 0x04;
        else if (str_equal(color_name, "white")) term_color = 0x0F;
        else term_print("Colors: blue, green, red, white");
    }
    else if (str_starts(input, "create ")) {
        for(int i=0; i<10; i++) {
            if(!disk[i].active) {
                str_copy(disk[i].name, input + 7);
                disk[i].active = true; disk[i].content[0] = '\0';
                term_print("File created."); break;
            }
        }
    } 
    else if (str_starts(input, "write ")) {
        char* name_start = input + 6; char* space = name_start;
        while(*space != ' ' && *space != '\0') space++;
        if(*space == ' ') {
            *space = '\0'; char* content = space + 1;
            for(int i=0; i<10; i++) {
                if(disk[i].active && str_equal(disk[i].name, name_start)) {
                    str_copy(disk[i].content, content);
                    term_print("Saved."); break;
                }
            }
        }
    }
    else if (str_starts(input, "run ")) {
        char* name = input + 4;
        for(int i=0; i<10; i++) {
            if(disk[i].active && str_equal(disk[i].name, name)) {
                if (str_starts(disk[i].content, "print ")) term_print(disk[i].content + 6);
                else term_print("No print command.");
                break;
            }
        }
    }
    else if (str_starts(input, "delete ")) {
        char* name = input + 7;
        for(int i=0; i<10; i++) {
            if(disk[i].active && str_equal(disk[i].name, name)) {
                disk[i].active = false; term_print("File Deleted."); break;
            }
        }
    }
    else if (input[0] != '\0') {
        term_print("Unknown Command.");
    }

    term_putc('\n');
    print_prompt();
}

/* --- 6. Keyboard Driver & Main Loop --- */
bool shift_active = false;
unsigned char kbd_low[128] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0};
unsigned char kbd_up[128]  = {0,27,'!','@','#','$','%','^','&','*', '(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0};

void kernel_main(void) {
    char buf[80]; int idx = 0;
    // Clear Screen at start
    for (size_t i = 0; i < 80 * 25; i++) VGA_BUFFER[i] = 0x0720; 

    term_print("Welcome to AmitOS \n");
    print_prompt();
    
    while(1) {
        if(inb(0x64) & 1) {
            uint8_t sc = inb(0x60);
            if (sc == 0x2A || sc == 0x36) shift_active = true;
            else if (sc == 0xAA || sc == 0xB6) shift_active = false;
            else if (!(sc & 0x80)) {
                char c = shift_active ? kbd_up[sc] : kbd_low[sc];
                if(c == '\n') { buf[idx] = '\0'; execute_command(buf); idx = 0; }
                else if(c == '\b' && idx > 0) { 
                    idx--; term_col--; 
                    VGA_BUFFER[term_row * 80 + term_col] = (uint16_t) ' ' | (uint16_t) term_color << 8; 
                }
                else if(idx < 79 && c >= ' ') { buf[idx++] = c; term_putc(c); }
            }
        }
    }
}