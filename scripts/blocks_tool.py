import argparse
import sys
import tomllib
from pathlib import Path
from typing import Any

from loguru import logger

VERSION = "0.0.1"
DATA_PATH = "assets/data/block"
TEXTURE_PATH = "assets/texture/block"


def show_data_list():
    work_path = Path().parent
    data_path = work_path / DATA_PATH
    blocks: list[dict[str, Any]] = []
    for block in data_path.rglob("*.toml"):
        if not block.is_file():
            continue
        if block.name == "template.toml":
            continue
        with open(block, "rb") as f:
            blocks.append(tomllib.load(f))
    blocks.sort(key=lambda x: x["id"])

    for block in blocks:
        print(f"id: {block['id']} name: {block['name']}")


def check_path():
    work_path = Path().parent
    logger.info(f"Work Path {work_path.resolve()}")
    logger.info(f"Script Dir {sys.path[0]}")

    data_path = work_path / DATA_PATH

    if not data_path.exists():
        logger.error(f"Blcoks Data Path {data_path} not Exists!")
    else:
        logger.info(f"Blocks Data Path {data_path}")

    texture_path = work_path / TEXTURE_PATH

    if not texture_path.exists():
        logger.error(f"Blocks Texture Path {texture_path} not Exists!")
    else:
        logger.info(f"Blocks Texture Path {texture_path}")


def handle_args(args: argparse.Namespace):
    if args.version:
        print(f"Blocks Tools: {VERSION}")
        print(f"Python: {sys.version}")
    if args.path:
        check_path()
    if args.list:
        show_data_list()


def init_parser(parser: argparse.ArgumentParser):
    parser.add_argument(
        "-v", "--version", action="store_true", help="Show Blocks Tools Version"
    )
    parser.add_argument(
        "--path", action="store_true", help="Check Blcoks Data and Texture Path"
    )
    parser.add_argument("-l", "--list", action="store_true", help="Show Blocks List")


def main():
    parser = argparse.ArgumentParser(description="Block Manage Tool")

    init_parser(parser)

    if len(sys.argv) == 1:
        parser.print_help()
        exit(0)

    args = parser.parse_args()
    handle_args(args)


if __name__ == "__main__":
    main()
