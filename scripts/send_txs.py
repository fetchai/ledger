import requests
import random
import string


from multiprocessing.dummy import Pool as ThreadPool

def submit(n):
    N = 20
    j = random.randint(0,7)
    print "Sending to ", 'http://localhost:%d/shard/submit-transaction' % (9090 + j)
    res1 = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
    res2 = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
    bod = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(N))
    r = requests.post('http://localhost:%d/shard/submit-transaction' % (9090 + j), json = {
        "resources": [res1, res2],
        "body":  bod
    })


def submitParallel(numbers, threads=2):
    pool = ThreadPool(threads)
    results = pool.map(submit, numbers)
    pool.close()
    pool.join()
    return results

if __name__ == "__main__":
    submitParallel(range(100), 2)

    
