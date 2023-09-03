const VERBOSE = false;

self.Module = {};

var is_ready;
var reply_when_ready;

function send_ready_message() {
    postMessage({ id: "ready" });
}

function on_message(ev) {
    const msg_id = ev.data.id;

    if (VERBOSE)
        console.debug("wrap_heavylifting_worker.js on_message",
            msg_id);

    if (msg_id == "reply-when-ready") {
        if (reply_when_ready) {
            console.error("wrap_heavylifting_worker.js on_message",
                "received more than one reply-when-ready");
        } else {
            reply_when_ready = true;
            console.debug("wrap_heavylifting_worker.js on_message",
                "reply_when_ready set to true");
            if (is_ready) {
                send_ready_message();
            }
        }
    } else if (msg_id == "frame_image_data") {
        process_frame(ev.data.frame_image_data);
    } else if (msg_id == "get_rom_data") {
        send_rom_data();
    } else {
        console.error("wrap_heavylifting_worker.js on_message",
            "received unexpected message data from parent", msg_id, ev.data);
    }
};

addEventListener("message", on_message);

function process_frame(frame_image_data) {
    const image_data_ptr = Module._malloc(frame_image_data.data.length);
    Module.HEAP8.set(frame_image_data.data, image_data_ptr);

    let result;
    try {
        if (VERBOSE)
            console.debug("calling heavylifting_process_frame",
                frame_image_data.width, frame_image_data.height, image_data_ptr);
        result = heavylifting_process_frame(
            frame_image_data.width, frame_image_data.height, image_data_ptr
        );
    } finally {
        Module._free(image_data_ptr);
    }
    if (VERBOSE)
        console.debug("heavylifting_process_frame returned:", result);
}

function send_rom_data() {
    if (VERBOSE)
        console.debug("send_rom_data");
    const rom_data_ptr = heavylifting_get_rom_data();
    const rom_data_not_unknown_min = heavylifting_get_rom_data_not_unknown_min();
    const rom_data_not_unknown_max = heavylifting_get_rom_data_not_unknown_max();

    let rom_data;
    if (rom_data_not_unknown_min < rom_data_not_unknown_max) {
        rom_data = new Int16Array(
            Module.HEAP8.buffer,
            rom_data_ptr,
            rom_data_not_unknown_max
        );
    } else {
        rom_data = [];
    }

    postMessage({ id: "rom_data", rom_data: rom_data });
}

var heavylifting_process_frame;
var heavylifting_get_rom_data;
var heavylifting_get_rom_data_not_unknown_min;
var heavylifting_get_rom_data_not_unknown_max;

self.Module.onRuntimeInitialized = function () {
    let heavylifting_init = Module.cwrap(
        "heavylifting_init", // name of C function
        "number", // return type
        ["number"], // argument types
    );

    let max_rom_size = 64 * 1024 * 1024;
    heavylifting_init(max_rom_size);

    heavylifting_process_frame = Module.cwrap(
        "heavylifting_process_frame", // name of C function
        "number", // return type
        ["number", "number", "number"], // argument types
    );

    heavylifting_get_rom_data = Module.cwrap(
        "heavylifting_get_rom_data",
        "number",
        [],
    );
    heavylifting_get_rom_data_not_unknown_min = Module.cwrap(
        "heavylifting_get_rom_data_not_unknown_min",
        "number",
        [],
    );
    heavylifting_get_rom_data_not_unknown_max = Module.cwrap(
        "heavylifting_get_rom_data_not_unknown_max",
        "number",
        [],
    );

    is_ready = true;
    if (reply_when_ready) {
        send_ready_message();
    }
};

importScripts("./heavylifting_main.js");
