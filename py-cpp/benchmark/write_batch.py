import sys
import time
import numpy as np
sys.path.append("/home/hyf/lab/GV/hyf/testShmQueue/test/shmqueue/")
from PyWapper.PyShmqueue import *

def measure_transfer_rate(writeQueue, data_size):
    data = np.random.bytes(data_size)
    print("len data : ", len(data))
    start_time = time.time()
    res = writeQueue.write(data)
    if not res:
        end_time = time.time()
        print("write successfully")
        transfer_time = end_time - start_time
        print("transfer_time: ", transfer_time)
        transfer_rate = data_size / transfer_time
        return transfer_rate
    else:
        return -1
if __name__ == "__main__":
    writeQueue = ShareMemoryQueue(12320,  4 * 1024 * 1024 * 1024, ONE_READ_ONE_WRITE, 1)

    data_size = 2 * 1024 * 1024 * 1024 

    transfer_rate = measure_transfer_rate(writeQueue, data_size)

    print(f"WRITE 512MB data: Transfer Rate: {transfer_rate} bps")
