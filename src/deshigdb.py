#
# gdb extension for providing debugging functionality for deshi things  
#

import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("deshi")

class vec2printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"({self.val['x']}, {self.val['y']})"
pp.add_printer("vec2", "^vec2$", vec2printer)

class textprinter:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return f"{self.val['buffer']}"
pp.add_printer("Text", "^Text$", textprinter)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)