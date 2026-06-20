"""
Batch upscale images using nearest neighbor interpolation (preserve hard pixel edges, no blur).

Usage:
    python upscale_nearest.py <input_folder> <output_folder> --scale 8
    python upscale_nearest.py <input_folder> <output_folder> --size 128

    --scale N    Upscale by factor N, e.g., 16x16 image --scale 8 -> 128x128
    --size N     Directly specify the target side length (square), e.g., --size 128
                 (--scale and --size are mutually exclusive; --size takes precedence)

Dependencies:
    pip install pillow
"""

import argparse
import os
import sys
from pathlib import Path

from loguru import logger
from PIL import Image

TEXTURE_PATH = "assets/texture/block"
TEMP_PATH = "pyout"

work_path = Path(__file__).parent.parent

input_path = work_path / TEXTURE_PATH
output_path = work_path / TEMP_PATH
skip_already_output = True


def upscale_image(
    in_path: str, out_path: str, scale: int | None, target_size: int | None
) -> bool:
    img = Image.open(in_path)
    w, h = img.size

    if target_size is not None:
        new_w, new_h = target_size, target_size
    else:
        if scale is None:
            raise ValueError("Either target_size or scale must be provided")
        new_w, new_h = w * scale, h * scale

    if skip_already_output:
        out = Path(out_path)
        if out.is_file():
            return False
    if w > new_w:
        logger.warning(f"w: {w} is greater than target: {new_w}")
    if h > new_h:
        logger.warning(f"h: {h} is greater than target: {new_h}")
    # NEAREST ensures crisp pixel boundaries without any blurring/interpolation artifacts

    out = img.resize((new_w, new_h), Image.NEAREST)  # type: ignore
    out.save(out_path)
    return True


def process_folder(
    scale: int | None,
    target_size: int | None,
    exts: tuple[str, ...],
) -> None:

    os.makedirs(output_path, exist_ok=True)

    processed = 0

    for root, _, files in os.walk(input_path):
        for fname in files:
            if not fname.lower().endswith(exts):
                continue

            in_file = os.path.join(root, fname)

            rel_path = os.path.relpath(root, input_path)
            out_dir = os.path.join(output_path, rel_path)
            os.makedirs(out_dir, exist_ok=True)

            out_file = os.path.join(out_dir, fname)

            ans = upscale_image(in_file, out_file, scale, target_size)
            processed += 1
            if ans:
                print(f"{os.path.relpath(in_file, input_path)}  ->  upscaled")

    if processed == 0:
        print(
            f"No images with extensions {exts} found in {input_path}.",
            file=sys.stderr,
        )
    else:
        print(f"\nDone, processed {processed} images, output to {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Batch nearest neighbor upscale (keep hard pixel edges)"
    )
    parser.add_argument("input_dir", nargs="?", default=None, help="Input image folder")
    parser.add_argument(
        "output_dir", nargs="?", default=None, help="Output image folder"
    )
    parser.add_argument(
        "--scale", type=int, default=None, help="Upscale factor, e.g., 8"
    )
    parser.add_argument(
        "--size", type=int, default=None, help="Target side length (square), e.g., 128"
    )
    parser.add_argument(
        "--ext",
        default=".png,.jpg,.jpeg,.bmp",
        help="File extensions to process, comma-separated (default: .png,.jpg,.jpeg,.bmp)",
    )
    parser.add_argument(
        "--noskip", action="store_true", help="Not skip the file if it is esisted"
    )
    args = parser.parse_args()

    if args.scale is None and args.size is None:
        print("Either --scale or --size must be specified.", file=sys.stderr)
        sys.exit(1)

    exts = tuple(e.strip().lower() for e in args.ext.split(","))
    global input_path
    global output_path
    global skip_already_output
    if args.input_dir:
        input_path = Path(args.input_dir)
    if args.output_dir:
        output_path = Path(args.output_dir)
    if args.noskip:
        skip_already_output = False
    process_folder(args.scale, args.size, exts)


if __name__ == "__main__":
    main()
