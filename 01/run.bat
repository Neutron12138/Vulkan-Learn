set filename=main
g++ -finput-charset=UTF-8 -fexec-charset=gbk ^
    "%filename%.cpp" -o "%filename%.exe" ^
    -lglfw3 -lgdi32 -lopengl32 -lglew32 -lfreeglut -lsfml-graphics -lsfml-window -lsfml-system -lvulkan-1 ^
    -I E:/C++/Project_Neutron/.release/
"%filename%.exe"