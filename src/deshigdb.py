#
# gdb extension for providing debugging functionality for deshi things  
#

import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("deshi")

class vec2printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        return (
            f"{{"
                f"x = {self.val['x']}, "
                f"y = {self.val['y']}"
            f"}}"
        )
pp.add_printer("vec2", "^vec2$", vec2printer)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)