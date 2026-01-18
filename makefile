src_demo:	main.c;
			cc -o src_demo main.c;
			./src_demo;
clean: 
		rm -f src_demo;