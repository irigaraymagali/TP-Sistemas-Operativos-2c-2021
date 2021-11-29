#Mate Dir
SRC_DIR=src
IDIR=./include
ODIR=obj
COMMONS=/usr/local/lib
CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt
EX_LIBS=-lcommons -lpthread -lreadline -lmate -lrt
USR=/usr
LOCAL_LIB=/local
LIB=/lib

USR_LIB=$(USR)$(LIB)

USR_LOCAL_LIB=$(USR)$(LOCAL_LIB)$(LIB)


# Shared Dir
SHARED_SRCDIR = ../shared/src
SHARED_IDIR = ../shared/include
SHARED_ODIR = ../shared/obj

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)

SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

# Create obj in Shared Library
$(SO_DIR)/%.o: 
	@mkdir -p $(SHARED_ODIR)

SHARED_OBJ_DIR = $(SO_DIR)/%.o

# sudo make install
install: $(SHARED_OBJ_DIR) $(SHARED_OBJ)
	# Compiling all .c    
	$(CC) -Wall -fPIC -c $(SRC_DIR)/*.c $(CFLAGS) $(LIBS)

	# Compiling all .o
	$(CC) -shared matelib.o $(SHARED_ODIR)/*.o -L$(COMMONS) $(LIBS) -o $(ODIR)/libmate.so

	# Moving the lib to the global link route
	cp -u $(ODIR)/libmate.so $(USR_LOCAL_LIB)
	cp -u $(ODIR)/libmate.so $(USR_LIB)
	cp -u $(ODIR)/libmate.so $(LIB)

	# Removing obj files..
	rm *.o
	# Done

# sudo make uninstall
uninstall:
	# Remove Libmate shared object    
	rm -f $(ODIR)/libmate.so
	rm -f $(USR_LOCAL_LIB)/libmate.so
	rm -f $(USR_LIB)/libmate.so
	rm -f $(LIB)/libmate.so

	# Done


# sudo make update
update: uninstall install

test: install
	$(CC) -g3 -Wall -L$(ODIR) -I$(IDIR) main.c $(EX_LIBS) -o example.out
