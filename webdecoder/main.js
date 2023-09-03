import { capture_frames } from "./jsmodules/capture_video.js";


const VERBOSE = false;


const worker_heavylifting = new Worker("./wrap_heavylifting_worker.js");


const video = document.createElement("video");
const canvas = document.getElementById("canvas");
const romout_binview = document.getElementById("romout");
const download_romdata_link = document.getElementById("download-romdata");

var is_heavylifting_ready = false;
var is_video_ready = false;

function set_heavylifting_ready() {
    console.info("set_heavylifting_ready");
    is_heavylifting_ready = true;
    check_start_capture_frame();
}

function set_video_ready() {
    console.info("set_video_ready");
    is_video_ready = true;
    check_start_capture_frame();
}

function check_start_capture_frame() {
    if (is_heavylifting_ready && is_video_ready) {
        do_start_capture_frame();
    }
}

var is_capture_frame_started = false;

function do_start_capture_frame() {
    if (is_capture_frame_started)
        return;
    console.info("do_start_capture_frame");
    is_capture_frame_started = true;

    capture_frames(video, canvas, 60, process_frame);

    send_get_rom_data_message();
}

function process_frame(frame_image_data) {
    if (VERBOSE)
        console.debug("main.js", "process_frame",
            frame_image_data.data, frame_image_data.data.buffer, frame_image_data.data.length);
    worker_heavylifting.postMessage(
        { id: "frame_image_data", frame_image_data: frame_image_data },
        [frame_image_data.data.buffer]
    );
}

function send_get_rom_data_message() {
    worker_heavylifting.postMessage({ id: "get_rom_data" });
}

function received_rom_data(rom_data) {
    if (VERBOSE)
        console.log("main.js", "received_rom_data", rom_data.length);

    const chars_per_line = 64;

    let text = "";
    for (let i = 0; i < rom_data.length; i++) {
        const b = rom_data[i];

        if (b == -1)
            text += "??";
        else
            text += b.toString(16).padStart(2, "0");
        text += " ";

        if (i % chars_per_line == (chars_per_line - 1))
            text += "\n";
    }

    while (romout_binview.firstChild)
        romout_binview.firstChild.remove();

    romout_binview.appendChild(document.createTextNode(text));

    const romdata_bytes = new Uint8Array(rom_data.length);
    for (let i = 0; i < rom_data.length; i++) {
        let b = rom_data[i];
        if (b == -1) {
            b = 0;
        }
        romdata_bytes[i] = b;
    }
    const blob = new Blob([romdata_bytes]);
    const prev_href = download_romdata_link.href;
    download_romdata_link.href = URL.createObjectURL(blob, { type: "application/octet-stream" });
    URL.revokeObjectURL(prev_href);

    setTimeout(send_get_rom_data_message, 1000);
}

if (!navigator.mediaDevices) {
    // note: file:// on firefox works
    alert("navigator.mediaDevices is undefined, are you using https (not http)?");
}

let constraints = {
    video: true,
    audio: false, // ?
};

navigator.mediaDevices
    .getUserMedia(constraints)
    .then((stream) => {
        console.info("Got video stream");

        // For displaying
        document.getElementById("video").srcObject = stream;

        // It turns out a video part of the document doesn't update when out of view,
        // meaning the frame we process freezes when scrolling down.
        // So use a script-created element that isn't part of the document
        // same issue as https://stackoverflow.com/questions/55484353/canvas-drawimage-of-autoplayed-video-only-works-when-video-element-is-visible
        video.srcObject = stream;

        const video_play_listener = function () {
            video.removeEventListener("play", video_play_listener);
            set_video_ready();
        }

        video.addEventListener("play", video_play_listener);

        video.play();
    })
    .catch((err) => {
        console.log(err);
        alert("error getting stream");
    });



function on_worker_heavylifting_message(ev) {
    const msg_id = ev.data.id;

    if (VERBOSE)
        console.debug("on_worker_heavylifting_message",
            msg_id);

    if (msg_id == "ready") {
        if (is_heavylifting_ready) {
            console.error("on_worker_heavylifting_message",
                "received more than one ready message");
            return;
        }
        console.debug("on_worker_heavylifting_message",
            "worker_heavylifting is ready");
        set_heavylifting_ready();
    } else if (msg_id == "rom_data") {
        received_rom_data(ev.data.rom_data);
    } else {
        console.error("on_worker_heavylifting_message",
            "received unexpected message data from worker_heavylifting", msg_id, ev.data);
    }
}

worker_heavylifting.addEventListener("message", on_worker_heavylifting_message);

worker_heavylifting.postMessage({ id: "reply-when-ready" });
