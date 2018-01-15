GLOBAL_REGISTER = {}

def Register(name):
    global GLOBAL_REGISTER
    if not name in GLOBAL_REGISTER:
        GLOBAL_REGISTER[name] = True
        return True
    return False
