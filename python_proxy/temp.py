class A:
    foo = 3
d = {}

def do():
    temp = A()
    d['temp'] = temp

    temp.foo = 234

    try:
        pass
    except:
        return
    finally:
        print("hiiiii")

    return

do()

for k, v in d.items():
    print(k, v.foo)


