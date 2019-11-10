namespace Backend
{
    [CCode (cname = "_KinesixDaemon")]
    [Compact]
    public class Interface
    {
        public enum SwipeDirection
        {
            SWIPE_UP,
            SWIPE_DOWN,
            SWIPE_LEFT,
            SWIPE_RIGHT
        }

        public enum PinchType
        {
            PINCH_IN,
            PINCH_OUT
        }

        [CCode (cname = "SwipedCallback")]
        public extern delegate void Swiped(SwipeDirection direction, int finger_count);

        [CCode (cname = "PinchCallback")]
        public extern delegate void Pinched(PinchType type, int finger_count);

        [CCode (cname = "kinesixd_daemon_new")]
        public extern Interface(Swiped swipe_cb, Pinched pinch_cb);

        [CCode (cname = "kinesixd_daemon_free")]
        private extern void free();

        ~Interface() { free(); }

        [CCode (cname = "kinesixd_daemon_get_valid_device_list")]
        public extern Device []get_available_devices();
    }
}