#include "sim_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
using namespace std;
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>

#define CLEAN_CHAR '0'
#define FRAME_NO_EXISTS_IN_RAM -1
#define  CODE_AREA_IN_RAM 0
#define  DATA_BSS_IN_RAM 1
#define HEAP_STACK_IN_RAM 2
#define OUT_LIMIT -2
#define CODE_AREA -10
#define IN_SWAP -20
#define DATA_BSS -30
#define HEAP_STACK -40
#define NOT_ALLOCATED -50

int execOrder[4];//array from frame to logic address
int beenHere=0;//flag that let us now if the memory start to be full
int Store=0;
int indexRam=0;
int numberPage=0;

sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size,
        int data_size, int bss_size, int heap_stack_size,
        int num_of_pages, int page_size){

    swapfile_fd=swap_file_name;
    program_fd=exe_file_name;
    this->text_size=text_size;
    this->data_size=data_size;
    this->bss_size=bss_size;
    this->page_size=page_size;
    this->heap_stack_size=heap_stack_size;
    this->num_of_pages=num_of_pages;
    this->page_table=new page_descriptor[num_of_pages];

    if(exe_file_name==NULL || swap_file_name==NULL){
        puts("name of one of the files is null");
        delete sim_mem;
        exit (EXIT_FAILURE);
    }
    this->program_fd = open(exe_file_name, O_RDONLY);
    if (this->program_fd < 0) {
        perror("Cannot open executable file! \n");
        exit(1);
    }
    this->swapfile_fd =open(swap_file_name, O_RDWR|O_CREAT|O_TRUNC ,0600);
    if (this->swapfile_fd < 0) {
        perror("Cannot open swap file! \n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < MEMORY_SIZE; i++)
        sim_mem->main_memory[i] = '0';
    for (int i = 0; i < num_of_pages; i++) {
        sim_mem->page_table[i].D = 0;
        sim_mem->page_table[i].frame = -1;
        sim_mem->page_table[i].V = 0;

        if(i < text_size/page_size)
        {sim_mem->page_table[i].P = 0;}
        else if( i > (text_size/page_size)-1 && i < (text_size + bss_size)/page_size)
        {sim_mem->page_table[i].P = 1;}
        else
        {sim_mem->page_table[i].P = 2;}
    }
    sim_mem->bss_size = bss_size ;
    sim_mem->heap_stack_size = heap_stack_size ;
    sim_mem->text_size = text_size ;
}
sim_mem::~sim_mem(){
    delete [] this->page_table;
    close(this->program_fd);
    close(this->swapfile_fd);
}
char sim_mem::load(int address){
    Store=0;
    int offset = address%page_size;
    int LogicPage=address/page_size;

    numberPage=LogicPage;
    int physicFrame;int physicAdd;

    int pagePlace=findPage(mem_sim,address,LogicPage);

    if(pagePlace==OUT_LIMIT)
        return '\0';
    if(pagePlace==NOT_ALLOCATED){

        perror("ERROR: THIS PAGE IS NOT ALLOCATED");
        return'\0';
    }

    if(pagePlace!=CODE_AREA_IN_RAM && pagePlace!= DATA_BSS_IN_RAM && pagePlace!=HEAP_STACK_IN_RAM){

        if(pagePlace==CODE_AREA)
        {
            enterToRam( sim_mem, LogicPage, CODE_AREA);//need to delete adress from function
        }
        else if(pagePlace== DATA_BSS )
        {
            enterToRam(sim_mem,LogicPage, DATA_BSS);
        }
        else if(pagePlace==HEAP_STACK)
        {
            enterToRam( sim_mem , LogicPage, HEAP_STACK);
        }
        else if(pagePlace==IN_SWAP)
        {
            enterToRam( sim_mem , LogicPage, IN_SWAP);
        }

    }
    physicFrame = get_frame_in_ram(sim_mem ,address);
    physicAdd = physicFrame*PAGE_SIZE + offset;

    return (sim_mem->main_memory[physicAdd]);
}
void sim_mem::store(int address, char value){
    int physicAdd,physicBlc;
    int numPage =address/page_size;

    numberPage=numPage;
    int offset = address %page_size;
    Store=1;
    int pagePlace= findPage(mem_sim,address,numPage);

    if(pagePlace==OUT_LIMIT)
        return;
    if(pagePlace==CODE_AREA || pagePlace==CODE_AREA_IN_RAM){
        perror("ERROR: read permission only");
        return;
    }

    if(pagePlace!=CODE_AREA_IN_RAM && pagePlace!= DATA_BSS_IN_RAM && pagePlace!=HEAP_STACK_IN_RAM){

        if(pagePlace==CODE_AREA)
        {
            enterToRam( sim_mem, numPage, CODE_AREA);//need to delete address from function
        }
        else if(pagePlace== DATA_BSS )
        {
            enterToRam(sim_mem ,numPage, DATA_BSS);
        }
        else if(pagePlace==HEAP_STACK)
        {
            enterToRam(sim_mem , numPage, HEAP_STACK);
        }
        else if(pagePlace==IN_SWAP)
        {
            enterToRam( sim_mem ,numPage, IN_SWAP);
        }



    }
    sim_mem->page_table[numPage].D=1;

    physicBlc = get_frame_in_ram(sim_mem ,address);
    physicAdd = physicBlc*PAGE_SIZE + offset;
    if(pagePlace!=CODE_AREA)
        sim_mem->main_memory[physicAdd]=value;


}
}
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
/************************************************************************************/
void sim_mem::print_swap() {
    char* str = (char*)malloc(this->page_size *sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
}
/***************************************************************************************/
void sim_mem::print_page_table() {
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
    for(i = 0; i < num_of_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[i].V,
               page_table[i].D,
               page_table[i].P,
               page_table[i].frame ,
               page_table[i].swap_index);
    }
}
int findPage( int address,int LogicalPage){
    int frame =get_frame_in_ram(sim_mem, address);//find the frame in the main memory (if exicst)
    /*
     * cuclate all the space that we have in memory(heap,stack,bss,data,text) and thst is ourlimit
     * */
    int limit_size = sim_mem->text_size + sim_mem->heap_stack_size + sim_mem->data_bss_size;

/**
 * if we have HIT! then we check what is the frame
 */


    if (address < 0 || address >= limit_size) {
        perror("ERROR: Outside the limit");
        return OUT_LIMIT;
    }
    /***
     * if page in ram we return what his area....
     */
    if (frame != FRAME_NO_EXISTS_IN_RAM) {
        if (sim_mem->page_table[LogicalPage].P == 0)
            return CODE_AREA_IN_RAM;

        else if (sim_mem->page_table[LogicalPage].P == 1)
            return DATA_BSS_IN_RAM;
        else if (sim_mem->page_table[LogicalPage].P == 2)
            return HEAP_STACK_IN_RAM;
    }
    /**
     * else we have **MISS**... we see that page not in ram , then decide from where to take him
     */
    if (sim_mem->page_table[LogicalPage].P == 0)//page in code
        return CODE_AREA;

    if (sim_mem->page_table[LogicalPage].P == 1)//data_bss check now if it dirty
    {
        if (sim_mem->page_table[LogicalPage].D == 1)
            return IN_SWAP;
        if (sim_mem->page_table[LogicalPage].D == 0)
            return DATA_BSS;
    }
    if (sim_mem->page_table[LogicalPage].P == 2)//stack or heap!
        return HEAP_STACK;


    return NOT_ALLOCATED;
}
void enterToRam(int LogicPage, int from) {
    int logicAddressBlock = LogicPage * PAGE_SIZE;

    int l=execOrder[beenHere];
    int offset=logicAddressBlock%PAGE_SIZE;

    int i;//the address of the page without offset
    char buffer[PAGE_SIZE + 1];
    buffer[PAGE_SIZE] = '\0';
    sim_mem->page_table[LogicPage].V = 1;
    sim_mem->page_table[LogicPage].frame = indexRam / PAGE_SIZE;
    int frame=mem_sim->page_table[LogicPage].frame;

    order++;
    int index = theNewOne();
    order_in_ram[LogicPage] = order;
    if (indexRam == 20 || beenHere != 0) {

        if (indexRam == 20)
            indexRam = 0;

        char buffer[PAGE_SIZE + 1];
        buffer[PAGE_SIZE] = '\0';
        int k = 0;
        for (int i = indexRam; i < indexRam + 5; i++) {
            buffer[k] = sim_mem->main_memory[i];
            k++;
        }

        order_in_ram[index] = 100;

        if (sim_mem->page_table[index].D == 1) {
            WriteToSwap(sim_mem, PAGE_SIZE, (l*PAGE_SIZE), buffer);

        }

        if (order_in_ram[index] == 100) {
            sim_mem->page_table[index].V = 0;
            sim_mem->page_table[index].frame = -1;
        }
        sim_mem->page_table[index].V = 0;
        sim_mem->page_table[index].frame = -1;

        sim_mem->page_table[LogicPage].frame = indexRam / PAGE_SIZE;
        beenHere++;


    }

    if ((from == CODE_AREA || from == DATA_BSS)) {
        //copy block to the ram:

        readFromExe(sim_mem, PAGE_SIZE, buffer, logicAddressBlock);

        for (i = 0; i < PAGE_SIZE; i++) {
            sim_mem->main_memory[indexRam + i] = buffer[i];
        }


    } else if (from == IN_SWAP) {
        readFromSwap(sim_mem, PAGE_SIZE,numberPage*PAGE_SIZE, buffer);

        for (i = 0; i < PAGE_SIZE; i++) {
            sim_mem->main_memory[indexRam + i] = buffer[i];
        }
    } else if ( from == HEAP_STACK) {

        for (i = 0; i < PAGE_SIZE; i++) {
            buffer[i] = '0';
            sim_mem->main_memory[indexRam + i] = buffer[i];
        }

    }
    else if(Store==0 &&(from == NOT_ALLOCATED ))
    {
        readFromExe(sim_mem, PAGE_SIZE, buffer, logicAddressBlock);
        for (i = 0; i < PAGE_SIZE; i++) {
            if (i == offset) { sim_mem->main_memory[indexRam + i] = buffer[i]; }
        }
    }
    execOrder[indexRam/5]=LogicPage;
    indexRam = indexRam + 5;

    numberPage=execOrder[indexRam/5];
}
int get_frame_in_ram( int address){
//calc logical address to physically address without offset (calc the place of the block)

    int numPage= address / PAGE_SIZE;
    if(sim_mem->page_table[numPage].V==1){//the page exists in ram
        return sim_mem->page_table[numPage].frame;
    }

    return FRAME_NO_EXISTS_IN_RAM;
}
int theNewOne(){
    int min = 26;
    int index =-1;
    for(int i =0 ; i< NUM_OF_PAGES ; i++){
        if(order_in_ram[i]<min){
            min = order_in_ram[i];
            index = i;}
    }
    return index;
}
void cleanSwap(){
    int i=0;
    char buffer[(SWAP_SIZE)+1];

    for(i=0;i<SWAP_SIZE;i++){
        buffer[i]=CLEAN_CHAR;
    }
    WriteToSwap(sim_mem,SWAP_SIZE, 0,buffer);
}
void WriteToSwap(int numOfWrite, int from, char *buffer){

    buffer[numOfWrite]='\0';
    if(lseek(sim_mem->swapfile_fd,from,SEEK_SET)!=from){
        clear_system(sim_mem);
        exit(-1);//check if need
    }
    if(write(sim_mem->swapfile_fd,buffer,numOfWrite)!=numOfWrite){
        clear_system(sim_mem);
        puts("write no success");
        exit (-1);
    }

}
void clear_system(){

    close(sim_mem->program_fd);
    close(sim_mem->swapfile_fd);
    delete sim_mem;
}//
void readFromSwap(int numOfRead, int from, char *buffer ){

    buffer[numOfRead]='\0';
    if(lseek(sim_mem->swapfile_fd,from,SEEK_SET)!=from){
        clear_system(sim_mem);
        perror("read no success");
        exit(-1);//check if need
    }
    if(read(sim_mem->swapfile_fd,buffer,numOfRead)!=numOfRead){
        clear_system(sim_mem);
        puts("read no success");
        exit (-1);//need exit if no read?
    }

}
void readFromExe(int numOfRead ,char *buffer,int logicAddressBlock ){

    buffer[numOfRead]='\0';
    if (lseek(sim_mem->program_fd, logicAddressBlock, SEEK_SET) != logicAddressBlock) {
        clear_system(sim_mem);
        exit(-1);//check if need
    }
    if (read(sim_mem ->program_fd, buffer, numOfRead) != numOfRead) {
        clear_system(sim_mem);
        puts("read no success");
        exit(-1);//need exit if no read?
    }

}