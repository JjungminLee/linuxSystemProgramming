CC = gcc

HEADER = ssu_header
SYNCC = ssu_sync
HELP = ssu_help
INIT = ssu_init
STRUCT = ssu_struct
ADD = ssu_add
REMOVE = ssu_remove
LIST = ssu_list

all : $(SYNCC) $(HELP) $(ADD) $(REMOVE) $(LIST) 

$(SYNCC) : $(SYNCC).o $(HEADER).o $(INIT).o $(STRUCT).o 
	$(CC) -g -o $@ $^ -lcrypto	

$(HELP) : $(HELP).o $(HEADER).o $(INIT).o $(STRUCT).o 
	$(CC) -g -o $@ $^ -lcrypto	

$(ADD) : $(ADD).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(REMOVE) : $(REMOVE).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(LIST) : $(LIST).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	


$(HEADER).o : $(HEADER).h $(HEADER).c
	$(CC) -c -o $@ $(HEADER).c -lcrypto

$(INIT).o : $(HEADER).h $(INIT).c
	$(CC) -c -o $@ $(INIT).c -lcrypto		

$(STRUCT).o : $(HEADER).h $(STRUCT).c
	$(CC) -c -o $@ $(STRUCT).c -lcrypto			

$(SYNCC).o : $(HEADER).h $(SYNCC).c
	$(CC) -c -o $@ $(SYNCC).c -lcrypto

$(HELP).o : $(HEADER).h $(HELP).c
	$(CC) -c -o $@ $(HELP).c -lcrypto

$(ADD).o : $(HEADER).h $(ADD).c
	$(CC) -c -o $@ $(ADD).c -lcrypto

$(REMOVE).o : $(HEADER).h $(REMOVE).c
	$(CC) -c -o $@ $(REMOVE).c -lcrypto	

$(LIST).o : $(HEADER).h $(LIST).c
	$(CC) -c -o $@ $(LIST).c -lcrypto	



clean :
	rm -rf $(SYNCC) $(HELP) $(ADD) $(REMOVE) $(LIST) 
	rm -rf *.o