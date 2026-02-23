#[no_mangle]
pub extern "C" fn update(dt: f32) {
    println!("[game:rust] update dt={:.3}", dt);
}

#[no_mangle]
pub extern "C" fn render() {
    println!("[game:rust] render");
}

#[no_mangle]
pub extern "C" fn get_score() -> i32 {
    100
}
