import requests
import json
import paramiko


# Transfer the file locally to aca nodes
def upload_file_aca(host, user, password, server_path, local_path, timeout=10):
    """
    :param host: 主机名
    :param user: 用户名
    :param password: 密码
    :param server_path: /root/alcor-control-agent/test/gtest
    :param local_path: ./text.txt
    :param timeout: 超时时间(默认)，int类型
    :return: bool
    """
    try:
        for host_ip in host:
            t = paramiko.Transport((host_ip, 22))
            t.banner_timeout = timeout
            t.connect(username=user, password=password)
            sftp = paramiko.SFTPClient.from_transport(t)
            sftp.put(local_path, server_path)
            t.close()
        return True
    except Exception as e:
        print(e)
        return False


# Execute remote SSH commands
def exec_sshCommand_aca(host, user, password, cmd, timeout=10):
    """
    :param host: 主机名
    :param user: 用户名
    :param password: 密码
    :param cmd: 执行的命令
    :param seconds: 超时时间(默认)，int类型
    :return: dict
    """
    result = {'status': [], 'data': []}  # Record return result
    try:
        # Create a SSHClient instance
        ssh = paramiko.SSHClient()
        ssh.banner_timeout = timeout
        # set host key
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        # Connect to remote server
        ssh.connect(host, 22, user, password, timeout=timeout)
        for command in cmd:
            # execute command
            stdin, stdout, stderr = ssh.exec_command(command, get_pty=True, timeout=timeout)
            # If need password
            if 'sudo' in command:
                stdin.write(password + '\n')
            # result of execution,return a list
            # out1 = stdout.readlines()
            out2 = stdout.read()
            # execution state:0 means success,1 means failure
            channel = stdout.channel
            status = channel.recv_exit_status()
            result['status'].append(status)
            # result['data'].append(out1)
            print(out2.decode())
        ssh.close()  # close ssh connection
        return result
    except Exception as e:
        print(e)
        return False


def talk_to_zeta(file_path, zgc_nodes_url):
    with open(file_path, 'r', encoding='utf8')as fp:
        zeta_data = json.load(fp)

    # create ZGC
    ZGC_data = json.dumps(zeta_data["ZGC_data"])
    requests.post(zgc_nodes_url[0] + "/zgcs", ZGC_data)
    requests.post(zgc_nodes_url[1] + "/zgcs", ZGC_data)

    # add VPC
    VPC_data = json.dumps(zeta_data["VPC_data"])
    response_data = requests.post(zgc_nodes_url[0] + "/vpcs", VPC_data).json()
    response_data = requests.post(zgc_nodes_url[1] + "/vpcs", VPC_data).json()

    # notify ZGC the ports created on each ACA
    PORT_data = json.dumps(zeta_data["PORT_data"])
    requests.post(zgc_nodes_url[0] + "/ports", PORT_data)
    requests.post(zgc_nodes_url[1] + "/ports", PORT_data)
    # TODO: 分别生成CHILD和PARENT的配置文件


def run():
    # # Call zeta API to create ZGC,vpc etc.and generate the information ACA need, and save it in zetaToAca_data.json
    file_path = './data/zeta_data.json'
    zgc1_nodes_url = ["http://172.16.62.247:8080", "http://172.16.62.248:8080"]
    talk_to_zeta(file_path, zgc1_nodes_url)

    aca_nodes = ['172.16.62.249', '172.16.62.250']
    user = '***'
    password = '***'
    server_path = '/root/alcor-control-agent/test/gtest'
    local_path = './zetaToAca_data.json'
    res = upload_file_aca(aca_nodes, user, password, server_path, local_path)
    if not res:
        print("upload file %s failed" % local_path)
    else:
        print("upload file %s successfully" % local_path)

    # Execute remote command, use the transferred file to change the information in aca_test_ovs_util.cpp,recompile using 'make',perform aca_test
    aca_nodes = ["172.16.62.249", "172.16.62.250"]
    cmd_list1 = ['cd alcor-control-agent;sudo make',
                 'cd alcor-control-agent;./build/tests/aca_tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_2_ports_CREATE_test_traffic_CHILD -p 10.213.43.187']
    result1 = exec_sshCommand_aca(host=aca_nodes[0], user=user, password=password, cmd=cmd_list1, timeout=10)
    cmd_list2 = ['cd alcor-control-agent;sudo make',
                 'cd alcor-control-agent;./build/tests/aca_tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_2_ports_CREATE_test_traffic_PARENT -c 10.213.43.188']
    result2 = exec_sshCommand_aca(host=aca_nodes[1], user=user, password=password, cmd=cmd_list2, timeout=10)
    print(result1["status"])
    print(result1["data"])
    print(result2["status"])
    print(result2["data"])


if __name__ == '__main__':
    run()