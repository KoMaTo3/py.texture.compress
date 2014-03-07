py.texture.compress
===================

C++ python module for converting images to DDS with compression (<code>DXT1</code>/<code>DXT3</code>/<code>DXT5</code>)

Can decode to RGBA-bytes: <code>tga</code>, <code>jpg</code>, <code>png</code>

Example:
```python
import imagecompress
imageRGBA = imagecompress.jpg2rgba( 'test.jpg' )
dxt1byteData = imagecompress.rgba2dxt1( imageRGBA )
```
where:
```python
imageRGBA = dict(
    width = pictureWidthInPixels, #int
    height = pictureHeightInPixels, #int
    data = RGBAbyteData, #bytes = 'RGBARGBARGBA...RGBA'
)
```

*NOTE: c++ code contains jpeglib, pnglib, zlib source-code.*
