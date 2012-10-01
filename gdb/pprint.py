class ExpPrettyPrinter():
    stringify = gdb.lookup_global_symbol('exp_stringify').value()

    def __init__(self, exp):
        self.exp = exp

    def to_string(self):
        return self.stringify(self.exp).string()


def yoshi_lookup_func(val):
    if str(val.type) == 'struct exp *':
        return ExpPrettyPrinter(val)

gdb.pretty_printers.append(yoshi_lookup_func)
print 'loaded Yoshi extensions'
