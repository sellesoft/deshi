#
# gdb extension for providing debugging functionality for deshi things  
#

import numpy
import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("deshi")

class vec2printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']})"
pp.add_printer("vec2", "^vec2$", vec2printer)

class vec3printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']}, {self.val['z']})"
pp.add_printer("vec3", "^vec3$", vec3printer)

class vec2iprinter:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']})"
pp.add_printer("vec2i", "^vec2i$", vec2iprinter)

class mat4printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        def e(x):
            return self.val["arr"][x]
        return str(numpy.matrix(
            [[e[ 0],e[ 1],e[ 2],e[ 3]],
             [e[ 4],e[ 5],e[ 6],e[ 7]],
             [e[ 8],e[ 9],e[10],e[11]],
             [e[12],e[13],e[14],e[15]]]))
pp.add_printer("mat4", "^mat4$", mat4printer)

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
        try:
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
                elif int(book["type"]) == int(gdb.parse_and_eval("RenderBookKeeper_Cmd")):
                    print(f"   cmd: c{book['cmd']} from {book['file']}:{book['line']}")
        except Exception as e:
            print(f"error: {e}")
render_bookkeeper()
        

class debug_ui(gdb.Command):
    def __init__(self):
        super(debug_ui, self).__init__("debug_ui", gdb.COMMAND_USER, gdb.COMPLETE_COMMAND)
    
    def invoke(self, arg, tty):
        g_ui = gdb.parse_and_eval("g_ui")
        idxarena = gdb.parse_and_eval("(u32*)g_ui->index_arena->start")
        items = []

        for i in range(g_ui['items']['count']):
            items.append(gdb.parse_and_eval(f"g_ui->items[{i}]"))
        for i in range(g_ui['immediate_items']['count']):
            items.append(gdb.parse_and_eval(f"g_ui->immediate_items[{i}]"))

        for item in items:
            print(f"{item['id']}--------------------------->")
            print(f" size: {item['size']}")
            print(f"scale: {item['scale']}")
            print(f"  pos:")
            print(f"     local: {item['pos_local']}")
            print(f"    screen: {item['pos_screen']}")
debug_ui()

class show_generic_heap(gdb.Command):
    def __init__(self):
        super(show_generic_heap, self).__init__("show_generic_heap", gdb.COMMAND_USER, gdb.COMPLETE_COMMAND)
        
    def invoke(self, arg, tty):
        try:
            heap = gdb.parse_and_eval("deshi__memory_generic_expose()")
            node = heap['last_chunk']
            out = []
            while 1:
                out.append(f"{node} : {node['size']}")
                node = node['prev']
                if not node: break
            print("\n".join(out))
        except Exception as e:
            print(f"error: {e}")
show_generic_heap()

class get_closest_generic_header(gdb.Command):
    def __init__(self):
        super(get_closest_generic_header, self).__init__("get_closest_generic_header", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, tty):
        try:
            val = gdb.parse_and_eval(arg)
            if val == None:
                print("invalid expression")
                return
            heap = gdb.parse_and_eval("deshi__memory_generic_expose()")
            node = heap['last_chunk']
            mindist = 999999999999999
            minnode = None
            print(val.type)
            u8type = gdb.lookup_type("u8").pointer()
            val = val.cast(u8type)
            while 1:
                dist = abs(val - node.cast(u8type))
                if dist < mindist:
                    mindist = dist
                    minnode = node
                node = node['prev']
                if not node: break
            # gdb.add_history(minnode)
            gdb.execute(f"p (MemChunk*){minnode}")
        except Exception as e:
            print(f"error: {e}")
get_closest_generic_header()

class catch_heap_corruption(gdb.Command):
    def __init__(self):
        super(catch_heap_corruption, self).__init__("chc", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
    def invoke(self, arg, tty):
        try:
            gdb.rbreak(r"*")
            for b in gdb.breakpoints():
                print(b.location)
                if "DEBUG" in b.location or "unicode" in b.location:
                    b.delete()
            
            while 1:
                gdb.execute("continue")
                if gdb.parse_and_eval("g_memory->generic_heap") != 0:
                    gdb.execute("call DEBUG_CheckHeap(g_memory->generic_heap)")
                

        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
catch_heap_corruption()
        
