import numpy as np
 
# 定义要生成的数据大小（以MB为单位）
sizes_mb = [5, 50, 100, 500]
 
# 每个向量中元素的数量（可以根据需要调整）
elements_per_vector = 100
 
# 数据类型（float32 每个元素占用4个字节）
dtype = np.float32
 
# 计算每个MB对应的向量数量（基于float32的大小）
vectors_per_mb = (1024 * 1024) // (elements_per_vector * dtype().nbytes)
 
# 生成并保存数据
for size_mb in sizes_mb:
    # 计算需要的向量数量
    num_vectors = int(size_mb * vectors_per_mb)
    
    # 生成随机向量数据
    vectors = np.random.rand(num_vectors, elements_per_vector).astype(dtype)
    
    # 构造文件名
    filename = f'vectors_{size_mb}MB.npy'
    
    # 保存数据到文件
    np.save(filename, vectors)
    
    # 输出确认信息
    print(f'Saved {filename} ({size_mb} MB)')
 
# 注意：这个脚本使用了numpy库来生成和保存数据。
# 如果你没有安装numpy，可以通过运行'pip install numpy'来安装它。