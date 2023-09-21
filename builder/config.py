import json

def read_config():
    with open("./build_config.json", "r+") as f:
        content = f.read()
        build_config = json.loads(content)
        print(build_config)

def write_config():
    pass

if __name__ == "__main__":
    read_config()
