import os
import tqdm


TOTAL_HASHES = 100 * int(1e6)

hash_check = set()
hashes = []

with tqdm.tqdm(total=TOTAL_HASHES) as pbar:
    while len(hashes) < TOTAL_HASHES:
        digest = os.urandom(32)
        if digest in hash_check:
            continue

        hash_check.add(digest)
        hashes.append(digest)
        pbar.update()


with open('output.bin', 'wb') as output_file:
    for raw_hash in hashes:
        output_file.write(raw_hash)
