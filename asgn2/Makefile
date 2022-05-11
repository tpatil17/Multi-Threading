CC	=	clang

CFLAGS	=	-g	-Wall	-Wextra	-Werror	-pedantic 

TARGET	=	httpserver
	
all:$(TARGET)

$(TARGET):	$(TARGET).c 
	$(CC)	$(CFLAGS)	-o	$(TARGET)	$(TARGET).c 

clean:	
	rm	-f	$(TARGET)	$(TARGET).o
	

