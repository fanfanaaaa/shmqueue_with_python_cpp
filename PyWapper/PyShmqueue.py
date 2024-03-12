import copy
import ctypes
import os
import time
import numpy as np
from ctypes import *
CACHE_LINE_SIZE = 64
# Define QueueModule
ONE_READ_ONE_WRITE = 0   
ONE_READ_MUL_WRITE = 1   
MUL_READ_ONE_WRITE = 2   
MUL_READ_MUL_WRITE = 3 

SHAR_KEY_MODEL = 100010
SHAR_KEY_TRAIN = 100020
SHAR_KEY_VAL = 100030
SHAR_KEY_TEST = 100040


class stMemTrunk(Structure):
    _fields_ = [("m_iBegin", ctypes.c_uint),
            ("__cache_padding1__", ctypes.c_char * CACHE_LINE_SIZE),
            ("m_iEnd", ctypes.c_uint),
            ("__cache_padding2__", ctypes.c_char * CACHE_LINE_SIZE),
            ("m_iShmKey", ctypes.c_int),
            ("__cache_padding3__", ctypes.c_char * CACHE_LINE_SIZE),
            ("m_iSize", ctypes.c_uint),
            ("__cache_padding4__", ctypes.c_char * CACHE_LINE_SIZE),
            ("m_iShmId", ctypes.c_int),
            ("__cache_padding5__", ctypes.c_char * CACHE_LINE_SIZE),
            ("m_eQueueModule", ctypes.c_int)]
    
class CShmRWlock(ctypes.Structure):
    _fields_ = [("m_iSemID", ctypes.c_int),
                ("m_iSemKey", ctypes.c_int)]
    
class CMessageQueue(ctypes.Structure):
    _fields_ = [("m_stMemTrunk", ctypes.POINTER(stMemTrunk)),
                ("m_pBeginLock", ctypes.POINTER(CShmRWlock)),
                ("m_pEndLock", ctypes.POINTER(CShmRWlock)),
                ("m_pQueueAddr", ctypes.POINTER(ctypes.c_ubyte)),
                ("m_pShm", ctypes.c_void_p)]

print("[Python]: Start loading libshmmqueue.so...")
libLoad = ctypes.cdll.LoadLibrary
try:
    module_root_path = os.path.dirname(__file__)
    share = libLoad(os.path.join(module_root_path, "libshmmqueue.so"))
    print("[Python]: Loading libshmmqueue.so successful!")
    # print(share.IsPowerOfTwo(3))
    # Define the function signatures
    share.CreateInstance.argtypes = [ctypes.c_int, ctypes.c_size_t, ctypes.c_int]
    share.CreateInstance.restype = ctypes.POINTER(CMessageQueue)

    share.GetInstance.argtypes = [ctypes.c_int, ctypes.c_size_t, ctypes.c_int]
    share.GetInstance.restype = ctypes.POINTER(CMessageQueue)

    share.DestroyInstance.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.DestroyInstance.restype = None

    share.SendMessage.argtypes = [ctypes.POINTER(CMessageQueue), ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t]
    share.SendMessage.restype = ctypes.c_int

    share.GetMessage.argtypes = [ctypes.POINTER(CMessageQueue), ctypes.POINTER(ctypes.c_ubyte)]
    share.GetMessage.restype = ctypes.c_int

    share.ReadHeadMessage.argtypes = [ctypes.POINTER(CMessageQueue), ctypes.POINTER(ctypes.c_ubyte)]
    share.ReadHeadMessage.restype = ctypes.c_int

    share.DeleteHeadMessage.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.DeleteHeadMessage.restype = ctypes.c_int

    share.PrintTrunk.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.PrintTrunk.restype = None

    share.GetFreeSize.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.GetFreeSize.restype = ctypes.c_uint

    share.GetDataSize.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.GetDataSize.restype = ctypes.c_uint

    share.GetQueueLength.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.GetQueueLength.restype = ctypes.c_uint

    share.InitLock.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.InitLock.restype = None

    share.IsBeginLock.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.IsBeginLock.restype = ctypes.c_int

    share.IsEndLock.argtypes = [ctypes.POINTER(CMessageQueue)]
    share.IsEndLock.restype = ctypes.c_int

    share.CreateShareMem.argtypes = [ctypes.c_int, ctypes.c_long, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    share.CreateShareMem.restype = ctypes.POINTER(ctypes.c_ubyte)

    share.DestroyShareMem.argtypes = [ctypes.c_void_p, ctypes.c_int]
    share.DestroyShareMem.restype = ctypes.c_int

    share.IsPowerOfTwo.argtypes = [ctypes.c_size_t]
    share.IsPowerOfTwo.restype = ctypes.c_int

    share.Fls.argtypes = [ctypes.c_size_t]
    share.Fls.restype = ctypes.c_int

    share.RoundupPowofTwo.argtypes = [ctypes.c_size_t]
    share.RoundupPowofTwo.restype = ctypes.c_size_t
    # share.get_share_body_address.restype = ctypes.POINTER(ctypes.c_uint8)
except Exception as e:
    print("[Python]: Failed to load shared memory dynamic library! Probably due to not placing libsharememory.so in the PyShareMemory module directory")
    print("[Python]: Detailed error messages-" + str(e))
    exit(1)




class ShareMemoryQueue:
    def __init__(self, shmkey, queuesize, queueModule, CREATE_FLAG):
        # get existed ShareMemoryQueue
        if(CREATE_FLAG):
            self.message_queue_ptr = share.CreateInstance(shmkey, queuesize, queueModule)
        else:
            self.message_queue_ptr = share.GetInstance(shmkey, queuesize, queueModule)
    
    def write(self, batch):
        bytes_batch = batch.tobytes()
        data_ptr = ctypes.cast(ctypes.create_string_buffer(bytes_batch), ctypes.POINTER(ctypes.c_ubyte))
        share.SendMessage(self.message_queue_ptr, data_ptr, len(bytes_batch))
    
    def get(self):
        buffer_size = 100  # Replace with the desired buffer size
        buffer = np.zeros(buffer_size, dtype=np.uint8)
        result = share.GetMessage(self.message_queue_ptr, buffer.ctypes.data_as(POINTER(c_ubyte)))
        print("Received data: ", buffer[:result])
       
    def GetDataSize(self):
        return share.GetDataSize(self.message_queue_ptr)

    def PrintTrunk(self):
        share.PrintTrunk(self.message_queue_ptr)
    
    def getQueueLength(self):
        share.GetQueueLength(self.message_queue_ptr)
    
    # def __del__(self):
    #     share.DestroyInstance(self.message_queue_ptr)
    #     print("Destroy ShareMemoryQueue")


def test():
    shmkey = 100010
    queuesize = 10240
    queueModule = ONE_READ_ONE_WRITE
    shmqueue = ShareMemoryQueue(shmkey, queuesize, queueModule)

    data = np.array([1, 2, 3], dtype=np.uint8)  # Assuming you have some data to write
    print(data.nbytes)
    shmqueue.write(data)
    shmqueue.write(data)
    shmqueue.write(data)
    shmqueue.write(data)
    shmqueue.write(data)
    shmqueue.write(data)
    shmqueue.PrintTrunk()

    shmqueue.get()
    shmqueue.PrintTrunk()

    # print(shmqueue.getQueueLength())
# test()