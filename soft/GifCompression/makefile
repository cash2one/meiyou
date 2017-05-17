OBJS = main.o

CompressGif : $(OBJS)
	g++ -o CompressGif $(OBJS) libfreeimage-3.17.0.so

main.o : main.cpp FreeImage.h stdtostring.h
	g++ -c main.cpp

.PHONY : clean
clean:
	-rm CompressGif $(OBJS)


