## Cycling in SDL GPU

Cycling means automatically creating a new instance of the resource if the existing resource is busy. Then those instances are preserved in a ring buffer.

A short summary taken from: https://moonside.games/posts/sdl-gpu-concepts-cycling/

> Just remember the following two rules when cycling a resource:
>
> * Previous commands using the resource have their data integrity preserved.
> * The data in the resource is undefined for subsequent commands until it is written to.
>
> Hopefully you now have an understanding of cycling and when to use it!
>
> To summarize some generally useful best practices:
>
> * For transfer buffers that are used every frame, cycle on the first Map call of the frame.
> * Cycle transfer buffers whenever they might be overwriting in-flight data.
> * For buffers that are overwritten every frame, cycle on the first upload of the frame.
> * For textures used in render passes and overwritten every frame, cycle on the first render pass usage of the frame.
> * Upload all dynamic buffer data early in the frame before you do any render or compute passes.
> * Do not cycle when you care about the existing contents of a resource.
