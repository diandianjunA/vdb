import numpy as np
import os

def generate_vector_file(file_name, file_size_mb, vector_dim=100):
    """
    生成指定大小的向量数据文件

    :param file_name: 输出文件名
    :param file_size_mb: 文件大小（单位：MB）
    :param vector_dim: 向量的维度
    """
    float_size = 4  # 32位float占4字节
    vectors_per_file = (file_size_mb * 1024 * 1024) // (vector_dim * float_size)
    
    # 生成随机向量数据
    vectors = np.random.rand(vectors_per_file, vector_dim).astype(np.float32)
    
    # 保存到文件
    with open(file_name, 'wb') as f:
        f.write(vectors.tobytes())

    print(f"Generated file: {file_name} with size {os.path.getsize(file_name) / (1024 * 1024):.2f} MB")

if __name__ == "__main__":
    # 文件名与大小配置
    file_configs = [
        ("vector_5MB.bin", 5),
        ("vector_50MB.bin", 50),
        ("vector_100MB.bin", 100),
        ("vector_500MB.bin", 500)
    ]
    
    for file_name, size_mb in file_configs:
        generate_vector_file(file_name, size_mb)