import os
import subprocess
import requests
import time
import threading
import queue
 
# 项目根目录
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
 
# 定义目录
build_dir = os.path.join(project_root, 'build')
 
# 编译C++代码
def compile_project():
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)
    cmake = ["cmake" ,".."]
    make = ["make" ,"-j4"]
    try:
        subprocess.run(cmake, check=True)
        subprocess.run(make, check=True)
        print("Compilation successful!")
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed: {e}")
 
# 运行可执行文件
def run_vdb_server(config):
    executable_path = os.path.join(build_dir, 'vdb_server')
    if os.path.exists(executable_path):
        try:
            vdb_server = subprocess.Popen(
                [executable_path, config],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )

            for line in iter(vdb_server.stdout.readline, ''):
                print(line, end="")
                if "created" in line :
                    input("vdb_server 启动成功，请输入回车键跳过...")
                    break

            return vdb_server
        except subprocess.CalledProcessError as e:
            print(f"Execution failed: {e}")
    else:
        print("Executable not found. Please compile the project first.")

# 运行可执行文件
def run_master_server():
    executable_path = os.path.join(build_dir, 'master_server')
    if os.path.exists(executable_path):
        try:
            master_server = subprocess.Popen(
                [executable_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            for line in iter(master_server.stdout.readline, ''):
                print(line, end="")
                if "created" in line :
                    input("master_server 启动成功，请输入回车键跳过...")
                    break

            return master_server
        except subprocess.CalledProcessError as e:
            print(f"Execution failed: {e}")
    else:
        print("Executable not found. Please compile the project first.")

# 运行可执行文件
def run_proxy_server(config):
    executable_path = os.path.join(build_dir, 'proxy_server')
    if os.path.exists(executable_path):
        try:
            proxy_server = subprocess.Popen(
                [executable_path, config],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            for line in iter(proxy_server.stdout.readline, ''):
                print(line, end="")
                if "created" in line :
                    input("proxy_server 启动成功，请输入回车键跳过...")
                    break

            return proxy_server
        except subprocess.CalledProcessError as e:
            print(f"Execution failed: {e}")
    else:
        print("Executable not found. Please compile the project first.")

def create_vdb_node(): 
    vdb_server1 = run_vdb_server(project_root + "/config/conf3.ini")
    vdb_server2 = run_vdb_server(project_root + "/config/conf4.ini")
    return vdb_server1, vdb_server2

def create_cluster():
    vdb_server1, vdb_server2 = create_vdb_node()
    print("vdb_server node has started")
    # 集群添加节点
    post({"operation": "add_follower","nodeId": 4, "endpoint": "127.0.0.1:9093"}, "http://localhost:8082/addFollower")
    print("raft has been created")
    get({}, "http://localhost:8082/listNode")
    master_server = run_master_server()
    print("master_server node has started")
    # 添加集群元数据
    post({"instanceId": "instance2", "nodeId": "node3", "url": "http://127.0.0.1:8082", "role": 0, "type": 1}, "http://localhost:6060/addNode")
    post({"instanceId": "instance2", "nodeId": "node4", "url": "http://127.0.0.1:9092", "role": 1, "type": 2}, "http://localhost:6060/addNode")
    # 查看所有节点信息
    get({"instanceId": "instance2"}, "http://localhost:6060/getInstance")
    # 启动代理服务器
    proxy_server = run_proxy_server(project_root + "/config/proxy_conf2.ini")
    print("proxy_server node has started")
    # input()
    # 查看拓扑结构
    get({}, "http://localhost:6061/topology")
    return vdb_server1, vdb_server2, master_server, proxy_server


def post(payload, url):
    """
    POST请求方法

    :param payload: 请求参数
    :param url: 请求url
    """
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"post successfully.")
            print({response.content})
        else:
            print(f"Failed to post. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error post: {e}")

def get(params, url):
    try:
        response = requests.get(url, params=params)
        if response.status_code == 200:
            print(f"get successfully.")
            print({response.content})
        else:
            print(f"Failed to get. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error post: {e}")
 
def test1():
    post({"operation": "insert", "object": {"id": 6, "vector": [0.9], "int_field": 49}}, "http://localhost:6061/insert")
    post({"operation": "search", "vector": [0.9], "k": 5}, "http://localhost:6061/search")
    post({"operation": "query", "id": 6,}, "http://localhost:6061/query")

# 主函数
def main():
    try:
        compile_project()
        vdb_server1, vdb_server2, master_server, proxy_server = create_cluster()
        test1()
    finally:
        vdb_server1.kill()
        vdb_server2.kill()
        master_server.kill()
        proxy_server.kill()

 
if __name__ == "__main__":
    main()