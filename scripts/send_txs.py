import requests
import random
import string


from multiprocessing.dummy import Pool as ThreadPool

def submit(n):
    N = 20
    j = random.randint(0,1)
    print "Sending to ", 'http://localhost:%d/group/submit-transaction' % (9090 + j)

    a = "0x0000"
    b = '0x0100'
    q = n & 3
    if q == 2:
        a = b
    elif q == 3:
        b = a
        
    res1 = a + ''.join(random.choice(['0', '1','2','3','4','5','6','7', '8', '9', 'A','B','C','D','E','F']) for _ in range(N))
    res2 = b + ''.join(random.choice(['0', '1','2','3','4','5','6','7', '8', '9', 'A','B','C','D','E','F']) for _ in range(N))
    bod = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))

    print "RESOURCE: ", res1, " ", res2
    r = requests.post('http://localhost:%d/group/submit-transaction' % (9090 + j), json = {
        "resources": [res1,res2],
        "body":  bod
    })
#    res1 = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
#    res2 = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
#    bod = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
    
#    r = requests.post('http://localhost:%d/group/submit-transaction' % (9090 + j + 1), json = {
#        "resources": [res1,res2],
#        "body":  bod
#    })    


def submitParallel(numbers, threads=2):
    pool = ThreadPool(threads)
    results = pool.map(submit, numbers)
    pool.close()
    pool.join()
    return results

if __name__ == "__main__":
    r = requests.post('http://localhost:9090/mining-power/1', json = { })
    r = requests.post('http://localhost:9091/mining-power/1', json = { })    
    submitParallel(range(10), 2)

