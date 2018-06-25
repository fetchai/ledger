class Type:
    def __init__(self, name):
        self.name_ = name

    def __str__(self):
        return self.name_

    
double = Type("libfetch.types.double")
float = Type("libfetch.types.float")
int = Type("libfetch.types.int")
