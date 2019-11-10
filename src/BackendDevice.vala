namespace Backend
{
    [CCode (cname = "_KinesixdDevice")]
    [Compact]
    public class Device 
    {
        [CCode (cname = "kinesixd_device_get_path")]
        private extern unowned string get_path();

        public unowned string path { get {
            return get_path();
        }}

        [CCode (cname = "kinesixd_device_new")]
        public extern Device(string path, string name, uint product_id, uint vendor_id);

        [CCode (cname = "kinesixd_device_free")]
        private extern void free();

        ~Device() { free(); }

        [CCode (cname = "kinesixd_device_equals")]
        public extern bool equals(Device other);
    }
}