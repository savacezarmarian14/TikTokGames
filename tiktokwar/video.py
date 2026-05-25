import subprocess
from pathlib import Path

OUTPUT = "intro_5sec.mp4"
WIDTH = 720
HEIGHT = 1280
FPS = 30
DURATION = 5
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def escape_drawtext(text: str) -> str:
    return (
        text.replace("\\", "\\\\")
            .replace(":", r"\:")
            .replace("'", r"\'")
    )


def main() -> None:
    if not Path(FONT).exists():
        raise FileNotFoundError(f"Font not found: {FONT}")

    ready_text = escape_drawtext("READY FOR WAR?")
    text_3 = escape_drawtext("3")
    text_2 = escape_drawtext("2")
    text_1 = escape_drawtext("1")

    vf = (
        "format=yuv420p,"
        f"drawtext=fontfile='{FONT}':"
        f"text='{ready_text}':"
        "fontcolor=white:fontsize=60:"
        "borderw=4:bordercolor=black:"
        "x=(w-text_w)/2:y=(h-text_h)/2:"
        "enable='between(t,0,2)',"
        f"drawtext=fontfile='{FONT}':"
        f"text='{text_3}':"
        "fontcolor=white:fontsize=180:"
        "borderw=6:bordercolor=black:"
        "x=(w-text_w)/2:y=(h-text_h)/2:"
        "enable='between(t,2,3)',"
        f"drawtext=fontfile='{FONT}':"
        f"text='{text_2}':"
        "fontcolor=white:fontsize=180:"
        "borderw=6:bordercolor=black:"
        "x=(w-text_w)/2:y=(h-text_h)/2:"
        "enable='between(t,3,4)',"
        f"drawtext=fontfile='{FONT}':"
        f"text='{text_1}':"
        "fontcolor=white:fontsize=180:"
        "borderw=6:bordercolor=black:"
        "x=(w-text_w)/2:y=(h-text_h)/2:"
        "enable='between(t,4,5)'"
    )

    cmd = [
        "ffmpeg",
        "-y",
        "-f", "lavfi",
        "-i", f"color=c=black:s={WIDTH}x{HEIGHT}:d={DURATION}:r={FPS}",
        "-vf", vf,
        "-t", str(DURATION),
        "-c:v", "libx264",
        "-pix_fmt", "yuv420p",
        OUTPUT,
    ]

    subprocess.run(cmd, check=True)
    print(f"Created: {OUTPUT}")


if __name__ == "__main__":
    main()