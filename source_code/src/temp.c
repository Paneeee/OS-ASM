int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);

	struct page_table_t * page_table = get_page_table(get_first_lv(address), proc->seg_table);

	int valid = 0;
	if(page_table != NULL){
		int i;
		for(i = 0; i < page_table->size; i++){
			if(page_table->table[i].v_index == get_second_lv(address)){
				addr_t physical_addr;
				if(translate(address, &physical_addr, proc)){
					int p_index = physical_addr >> OFFSET_LEN;
					int num_free_pages = 0;
					addr_t cur_vir_addr = (num_free_pages << OFFSET_LEN) + address;
					addr_t seg_idx,page_idx;
					do{
						_mem_stat[p_index].proc = 0;
						int found = 0;
						int k;
						seg_idx=get_first_lv(cur_vir_addr);
						page_idx=get_second_lv(cur_vir_addr);
						for(k = 0; k < proc->seg_table->size && !found; k++){
							if( proc->seg_table->table[k].v_index == seg_idx ){
								int l;
								for(l = 0; l < proc->seg_table->table[k].pages->size; l++){
									if(proc->seg_table->table[k].pages->table[l].v_index== page_idx){
										int m;
										for(m = l; m < proc->seg_table->table[k].pages->size - 1; m++)//Rearrange page table
											proc->seg_table->table[k].pages->table[m]= proc->seg_table->table[k].pages->table[m + 1];
										
										proc->seg_table->table[k].pages->size--;
										if(proc->seg_table->table[k].pages->size == 0){//If page empty
											free(proc->seg_table->table[k].pages);
											for(m = k; m < proc->seg_table->size - 1; m++)//Rearrange segment table
												proc->seg_table->table[m]= proc->seg_table->table[m + 1];
											proc->seg_table->size--;
										}
										found = 1;
										break;
									}
								}
							}
						}

						p_index = _mem_stat[p_index].next;
						num_free_pages++;
					}
					while(p_index != -1);
					valid = 1;
				}
				break;
			}
		}
	}

	pthread_mutex_unlock(&mem_lock);

	if(!valid)
		return 1;
	else
		return 0;
}
