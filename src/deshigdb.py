#
# gdb extension for providing debugging functionality for deshi things  
#

import gdb
# import tkinter as tk
pp = gdb.printing.RegexpCollectionPrettyPrinter("deshi")

class vec2printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']})"
pp.add_printer("vec2", "^vec2$", vec2printer)

class vec2iprinter:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']})"
pp.add_printer("vec2i", "^vec2i$", vec2iprinter)

class textprinter:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"{self.val['buffer']}"
pp.add_printer("Text", "^Text$", textprinter)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

class render_bookkeeper(gdb.Command):
    def __init__(self):
        super(render_bookkeeper, self).__init__("rbookkeep", gdb.COMMAND_USER, gdb.COMPLETE_COMMAND)
    def invoke(self, arg, tty):
        val = gdb.lookup_symbol("renderBookKeeperArray")[0]
        if val == None:
            print("Could not find the symbol 'renderBookKeeperArray")
            return
        count = gdb.lookup_symbol("renderBookKeeperCount")[0]
        if count == None:
            print("Could not find symbol 'renderBookKeeperCount")
            return
        vertex_start = int(gdb.parse_and_eval("(Vertex2*)renderTwodVertexArray"))
        vertex_size = int(gdb.parse_and_eval("sizeof(Vertex2)"))
        index_start = int(gdb.parse_and_eval("(u32*)renderTwodIndexArray"))
        index_size = int(gdb.parse_and_eval("sizeof(u32)"))
        for i in range(int(count.value())):
            book = gdb.parse_and_eval(f"renderBookKeeperArray[{i}]")
            if int(book["type"]) == int(gdb.parse_and_eval("RenderBookKeeper_Vertex")):
                file = book['file']
                line = book['line']
                start = str((int(book['vertex']['start']) - vertex_start)/vertex_size)
                count = float(book['vertex']['count'])
                print(f"vertex: s{start} c{count} from {file}:{line}")
            elif int(book["type"]) == int(gdb.parse_and_eval("RenderBookKeeper_Index")):
                file = book['file']
                line = book['line']
                start = str((int(book['index']['start']) - index_start)/index_size)
                count = float(book['index']['count'])
                print(f" index: s{start} c{count} from {file}:{line}")
            # elif int(book["type"]) == int(gdb.parse_and_eval("RenderBookKeeper_Cmd")):
            #     print(f"   cmd: c{book['cmd']} from {book['file']}:{book['line']}")
render_bookkeeper()
        

# class debug_ui(gdb.Command):
#     def __init__(self):
#         super(debug_ui, self).__init__("debug_ui", gdb.COMMAND_USER, gdb.COMPLETE_COMMAND)
    
#     def invoke(self, arg, tty):
#         window = tk.Tk()
#         greet = tk.Label(text="hi")
#         greet.pack()
# debug_ui()