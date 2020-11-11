# Glitch Suite - PNG
Glitch Suite is a series of tools created for the sole purpose of manipulating image data.

This particular tool, created back in 2017, is specifically for modifying PNG images, and consists of a modified libPNG codec to control various aspects of the coding and decoding process.

## Building
Should just need to run these commands afaik:
```
git clone https://github.com/KernelEquinox/glitch_png.git
cd glitch_png
make
```

## Usage
```
Glitch Suite - PNG

Usage: glitch_png [options] <infile> <outfile>

Options:
    -a <filter>   Filter to use for writing row data
    -d <filter>   Filter to use for decoding row data
    -h            Show this help message
    -i            Interlace the image data
    -p <num>      Interlace pass to stop at (1-7)
    -s            Stop the interlace process mid-pass
    -x <byte>     Byte mask to modify filter with
    -y <byte>     Byte mask for AVG / PAETH output
    -z <num>      Force alpha channel off (0) or on (1)

Filters:
    0:   None
    1:   Sub
    2:   Up
    3:   Average
    4:   Paeth
    5:   Sub (Modified)
    6:   Up (Modified)
    7:   Average (Modified)
    8:   Paeth (Modified)
    255: Random (Output only)

Examples:

    Write with Sub, read with Up:
        glitch_png -a 1 -d 2 in.png out.png

    Write with Up (Modified), AND each pixel with 0xFE, read with Up:
        glitch_png -a 6 -d 2 -x 0xFE in.png out.png

    Write with Up, read with Paeth, interlace up to pass 2 and stop mid-pass:
        glitch_png -a 2 -d 4 -i -p 2 -s in.png out.png

    Write normally, read randomly:
        glitch_png -d 255 in.png out.png
```

## Examples

Base image:
<br>![base](https://user-images.githubusercontent.com/15955749/98867559-5e10f100-2434-11eb-9c4e-7981abe64673.png)

Write raw data without transparency, then read with Up:
```
glitch_png -a 0 -d 2 -z 0 base.png none_up.png
```
![none_up](https://user-images.githubusercontent.com/15955749/98867613-784acf00-2434-11eb-8851-51668c196f5f.png)

Write interlaced data using the Up filter, stop in the middle of pass 4, then read with Paeth:
```
glitch_png -a 2 -d 4 -z 0 -i -p 4 -s base.png interlace.png
```
![interlace](https://user-images.githubusercontent.com/15955749/98867588-6ec16700-2434-11eb-9c84-51d4e6029b17.png)

Write interlaced data, stop in the middle of pass 7, then read with randomized filters:
```
glitch_png -d 255 -z 0 -i -p 7 -s base.png random.png
```
![random](https://user-images.githubusercontent.com/15955749/98867633-81d43700-2434-11eb-8e5a-c6d029eda36d.png)

Write data using the Up filter, `AND` each byte with `0xFE`, then read with Up:
```
glitch_png -a 6 -d 2 -x 0xFE base.png up_mod.png
```
![up_mod](https://user-images.githubusercontent.com/15955749/98867654-8993db80-2434-11eb-8e9b-0d5ae5730d7d.png)