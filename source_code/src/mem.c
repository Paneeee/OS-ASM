
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t  * seg_table) { // first level table
	
	int i;
	for (i = 0; i < seg_table->size; i++) {
		if (seg_table->table[i].v_index == index){
			return seg_table->table[i].pages;
		}
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* Found correct page entry. */
			*physical_addr = (page_table->table[i].p_index*PAGE_SIZE 
							+ offset);
			return 1;
		}
	}
	
	return 0;	
}

/* Alloc [size] byte for [proc], return first byte address in [ret_mem].*/
addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);	// Lock memory.
	addr_t ret_mem = 0; 

	uint32_t number_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE :
		size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	int counter = 0;//First page of process NULL => no page stored

	/* Check availability of RAM space. */
	for(int i = 0; i < NUM_PAGES && counter < number_pages; i++){
		if(_mem_stat[i].proc == 0){
			counter++;
		}
	}
	if (counter == number_pages 
		&& (proc->bp + number_pages*PAGE_SIZE <=RAM_SIZE)){
		mem_avail = 1;	
	}

	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += number_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		int prev_index, cur_index = 0; //Used to update in _mem_stat
		addr_t segment_index, page_index; //Used to locate in seg_table
		addr_t temp_address;		   //Current virtual address
		struct seg_table_t* segment_table = proc->seg_table;

		for (int i = 0; i<NUM_PAGES; i++) {
			if (_mem_stat[i].proc == 0){
				/* Allocatable frame. */
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = cur_index;
				
				if (cur_index!=0){
					/* Update [next] of previsited frame. */
					_mem_stat[prev_index].next = i;
				}

				/* Find current virtual address.*/
				temp_address = ret_mem + (cur_index * PAGE_SIZE);
				/* The first layer index */
				segment_index = get_first_lv(temp_address);
				/* The first layer index */
				page_index = get_second_lv(temp_address);

				if (segment_table->table[0].pages == NULL)
					segment_table->size = 0;
				
				int flag = 0; //Entry with [segment_index] exists?

				for (int j = 0; j < segment_table->size; j++){
					if (segment_table->table[j].v_index == segment_index){
						/* Found correct entry. */
						struct page_table_t *page_table =
										segment_table->table[j].pages;
						int table_size = page_table->size;

						/* Update new entry in page table and table size. */
						page_table->table[table_size].p_index=i;
						page_table->table[table_size].v_index=page_index;

						page_table->size++;
						flag = 1;

						break;
					}
				}

				if (!flag){
					/* Cannot found entry has [segment_index]. */
					/* Update new entry in seg table, new entry in page table. */
					segment_table->table[segment_table->size].v_index = segment_index;
					segment_table->table[segment_table->size].pages =
						(struct page_table_t*)malloc(sizeof(struct page_table_t));

					segment_table->table[segment_table->size].pages->table[0].v_index = page_index;
					segment_table->table[segment_table->size].pages->table[0].p_index = i;

					segment_table->table[segment_table->size].pages->size = 1;
					segment_table->size++;
				}
				/* Update location of previsited frame and current page of [proc]. */
				prev_index = i;
				cur_index++;
				if (cur_index == number_pages){
					_mem_stat[i].next = -1;
					break;
				}
			}
		}
	}
		
	pthread_mutex_unlock(&mem_lock); //Unlock memory.
	return ret_mem;
}

/*Free memory allocated for [proc] starts at [address]. */
int free_mem(addr_t address, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);	//Lock memory region.
	
	struct page_table_t* test =  get_page_table(
						get_first_lv(address),
						proc->seg_table); //Search in first level

	if (test != NULL){
		/* Exist corresponding segment (entry in seg table) . */
		int count = 0;
		addr_t physical_address;
		/* Get physical address. */
		translate(address, &physical_address, proc);	
		/* Get location of first frame store data of [address]. */
		int index = physical_address >> OFFSET_LEN;

		while(index!=-1){
			/* Get current virtual address. */
			addr_t current_address = address + count * PAGE_SIZE;
			/* The first layer index */
			addr_t segment_index = get_first_lv(current_address);
			/* The second layer index */
			addr_t page_index = get_second_lv(current_address);

			int flag = 0; //Found and remove corresponding page in seg_table?
			
			for (int i=0; i<proc->seg_table->size && !flag; i++){
				if (proc->seg_table->table[i].v_index == segment_index){
					/* Found correct segment entry. */
					struct page_table_t* page_table = proc->seg_table->table[i].pages;
					for (int j=0; j<page_table->size;j++){
						if (page_table->table[j].v_index == page_index){
							/* Found correct page entry. */
							
							/* Remove page entry and update page table size. */
							for (int p=j; p<page_table->size-1;p++){
								page_table->table[p] = page_table->table[p+1];	
							}
							page_table->size--;

							if (page_table->size==0){				
								/* No more page entry in current segment entry. */

								/* Free [pages] pointer. */
								free(proc->seg_table->table[i].pages);
								/* Remove current segment entry and update seg table size. */
								for (int q=i;q<proc->seg_table->size-1;q++){
									proc->seg_table->table[q] =proc->seg_table->table[q+1];	
								}
								proc->seg_table->size--;

							}
							flag = 1;
							break;
						}
					}
					
				}
			}
			count++;

			/* Free frame corresponding to current page. */			
			_mem_stat[index].proc = 0;
			index = _mem_stat[index].next;
		}
	}
	pthread_mutex_unlock(&mem_lock);	// Unlock memory region. 
	return 0;
}

/*Access data at physical address corresponding to [address]
* in [proc], then save to [data]. */
int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
}
/* Access data at physical address corresponding to [address] 
* in [proc], then assign [data] to it. */
int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}else{
		return 1;
	}
}

/* Display RAM status. */
void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


