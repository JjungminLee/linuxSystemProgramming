CC = gcc

HEADER = ssu_header
REPO = ssu_repo
HELP = ssu_help
INIT = ssu_init
STRUCT = ssu_struct
ADD = ssu_add
REMOVE = ssu_remove
COMMIT = ssu_commit
STATUSS = ssu_status
REVERT = ssu_revert
LOG = ssu_log

all : $(REPO) $(HELP) $(ADD) $(REMOVE) $(COMMIT) $(STATUSS) $(REVERT) $(LOG)

$(REPO) : $(REPO).o $(HEADER).o $(INIT).o $(STRUCT).o 
	$(CC) -g -o $@ $^ -lcrypto	

$(HELP) : $(HELP).o
	$(CC) -g -o $@ $^ -lcrypto	

$(ADD) : $(ADD).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(REMOVE) : $(REMOVE).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(COMMIT) : $(COMMIT).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(STATUSS) : $(STATUSS).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(REVERT) : $(REVERT).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto	

$(LOG) : $(LOG).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto

$(REPO).o : $(HEADER).h $(REPO).c
	$(CC) -c -o $@ $(REPO).c -lcrypto

$(HELP).o : $(HELP).h $(HELP).c
	$(CC) -c -o $@ $(HELP).c -lcrypto

$(HEADER).o : $(HEADER).h $(HEADER).c
	$(CC) -c -o $@ $(HEADER).c -lcrypto

$(INIT).o : $(HEADER).h $(INIT).c
	$(CC) -c -o $@ $(INIT).c -lcrypto

$(STRUCT).o : $(HEADER).h $(STRUCT).c
	$(CC) -c -o $@ $(STRUCT).c -lcrypto	

$(ADD).o : $(HEADER).h $(ADD).c
	$(CC) -c -o $@ $(ADD).c -lcrypto

$(REMOVE).o : $(HEADER).h $(REMOVE).c
	$(CC) -c -o $@ $(REMOVE).c -lcrypto	

$(COMMIT).o : $(HEADER).h $(COMMIT).c
	$(CC) -c -o $@ $(COMMIT).c -lcrypto	

$(STATUSS).o : $(HEADER).h $(STATUSS).c
	$(CC) -c -o $@ $(STATUSS).c -lcrypto	

$(REVERT).o : $(HEADER).h $(REVERT).c
	$(CC) -c -o $@ $(REVERT).c -lcrypto	

$(LOG).o : $(HEADER).h $(LOG).c
	$(CC) -c -o $@ $(LOG).c -lcrypto


clean :
	rm -rf $(REPO) $(HELP) $(ADD) $(REMOVE) $(COMMIT) $(STATUSS) $(REVERT) $(LOG)
	rm -rf *.o