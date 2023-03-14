//Jennifer

#ifndef EX6_SIM_MEM_H
#define EX6_SIM_MEM_H

#define MEMORY_SIZE 200
extern char main_memory[MEMORY_SIZE];
using namespace std;

typedef struct page_descriptor
{
    int V; // valid
    int D; // dirty
    int P; // permission
    int frame; //the number of a frame if in case it is page-mapped
    int swap_index; // where the page is located in the swap file.
} page_descriptor;

class sim_mem {
    int swapfile_fd; //swap file fd
    int program_fd; //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;
    page_descriptor *page_table; //pointer to page table
public:
    sim_mem(char exe_file_name[], char swap_file_name[], int text_size,
                     int data_size, int bss_size, int heap_stack_size,
                     int num_of_pages, int page_size);
    ~sim_mem();
    char load(int address);
    void store(int address, char value);
    void print_memory();
    void print_swap ();
    void print_page_table();
private:
    //the order that the pages entered to RAM
    int theNewOne();
    int order;
    int indexRam;
    int order_in_ram[25]; //aray that updating the order of entry to RAM
    void cleanSwap();
    int findPage(int address, int LogicalPage);
    void enterToRam(int LogicPage, int from);
    int get_frame_in_ram( int address);
    void readFromExe(int numOfRead ,char *buffer,int logicAddressBlock );
    void WriteToSwap(int numOfWrite, int from, char *buffer);
    void readFromSwap(int numOfRead, int from, char *buffer );
    void clear_system();
};


#endif //EX6_SIM_MEM_H
