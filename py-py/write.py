import sys
sys.path.append("/home/hyf/lab/GV/hyf/testShmQueue/test/shmqueue/")
from PyWapper.PyShmqueue import *
from time import sleep
import numpy as np

arr1 = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], dtype=np.uint8)
arr2 = np.array([2, 4, 6, 8, 10, 12, 14, 16, 18, 20], dtype=np.uint8)

writeQueue = ShareMemoryQueue(123, 2 * 80, ONE_READ_ONE_WRITE, 1)
print("write arr1: ", arr1)
print("write arr2: ", arr2)
writeQueue.write(arr1)
print(f"arr1 size: {arr1.size}, arr2 size: {arr2.size}")
writeQueue.write(arr2)
writeQueue.PrintTrunk()



