import requests
import random
import string


from multiprocessing.dummy import Pool as ThreadPool

def submit(n):
    N = 20
    j = random.randint(0,1)
    lanes = 2
    print "Sending to ", 'http://localhost:%d/group/submit-transaction' % (9590 + j)

    a = "0x0000"
    b = '0x0100'
    q = n & 3
    if q == 2:
        a = b
    elif q == 3:
        b = a

    resources = []
    hexChars = ['0', '1','2','3','4','5','6','7', '8', '9', 'A','B','C','D','E','F']
    for i in range(2):
        NN = random.randint(0, lanes -1 )
        a = hexChars[ (NN >> 4)& 15 ] + hexChars[ NN & 15] + hexChars[ (NN >> 12)& 15 ] + hexChars[ (NN >> 8)& 15 ];
        print a
        res1 = "0x" +a + ''.join(random.choice(['0', '1','2','3','4','5','6','7', '8', '9', 'A','B','C','D','E','F']) for _ in range(N))
        print res1
        resources.append(res1)
    bod = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))

    for k in range(lanes):
        r = requests.post('http://localhost:%d/group/submit-transaction' % (9590 + k + j*lanes), json = {
            "resources": resources,
            "body":  bod
        })

    

    

def submitParallel(numbers, threads=2):
    pool = ThreadPool(threads)
    results = pool.map(submit, numbers)
    pool.close()
    pool.join()
    return results

if __name__ == "__main__":
    r = requests.post('http://localhost:9590/mining-power/1', json = { })
    r = requests.post('http://localhost:9591/mining-power/1', json = { })    
    submitParallel(range(100), 20)

