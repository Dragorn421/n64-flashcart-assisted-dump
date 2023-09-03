function get_frame_image_data(video, canvas) {
    const int_scale = 1;

    // video width/height
    const vw = video.videoWidth;
    const vh = video.videoHeight;

    if (vw == 0 || vh == 0)
        return null;

    // canvas width/height
    const cw = vw * int_scale;
    const ch = vh * int_scale;

    canvas.width = cw;
    canvas.height = ch;

    const ctx = canvas.getContext("2d");
    ctx.drawImage(
        video,
        0, 0, vw, vh,
        0, 0, cw, ch,
    );

    if (0) {
        ctx.fillStyle = "white";
        const nrows = 5;
        ctx.fillRect(0, ch - nrows, cw, nrows);
    }

    const frame_image_data = ctx.getImageData(0, 0, cw, ch);

    return frame_image_data;
}

function capture_frames(video, canvas, fps, callback) {
    return setInterval(function () {
        const frame_image_data = get_frame_image_data(video, canvas);
        if (frame_image_data !== null)
            callback(frame_image_data);
    }, 1000 / fps);
}

export { capture_frames };
