import sys
sys.path.append("/home/hyf/lab/GV/hyf/testShmQueue/test/shmqueue/")
from PyWapper.PyShmqueue import *
from time import sleep
import torch

# arr1 = torch.tensor([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], dtype=torch.int64)
# arr2 = torch.tensor([2, 4, 6, 8, 10, 12, 14, 16, 18, 20], dtype=torch.int64)

# print(arr1)

# print(f"arr1 size: {arr1.numel() * arr1.element_size()}")

readQueue = ShareMemoryQueue(123, 2 * 80, ONE_READ_ONE_WRITE, 0)

readQueue.get()
readQueue.PrintTrunk()

readQueue.get()
readQueue.PrintTrunk()

