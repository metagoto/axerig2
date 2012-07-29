
#include <linux/input.h>
#include <alsa/asoundlib.h>
#include <signal.h>
#include <iostream>
#include <array>
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


using std::cout;
using std::endl;


typedef std::array<unsigned char, 3> switch_state; // CC, toggle, state

struct config
{
    std::string device;
    std::string portname;
    unsigned char channel;
    std::unordered_map<int, switch_state> switch_map;
    unsigned char pedal_cc;
};


bool load_config(config& conf)
{
    using namespace boost::property_tree;
    ptree pt;

    try {
        json_parser::read_json("config.json", pt);
        conf.device = pt.get<std::string>("rig_device");
        conf.portname = pt.get("axe_midi.port", "AXE-FX II");
        conf.channel = pt.get<unsigned char>("axe_midi.channel", 0);
    }
    catch (std::exception& e) {
        cout << "config error: " << e.what() << endl;
        return false;
    }

#define AXE_SW_CONF(num) \
  { auto ppt = pt.get_child_optional("switch."#num); \
    if (ppt) { \
        boost::optional<unsigned char> cc = ppt->get_optional<unsigned char>("cc"); \
        if (cc) { \
            unsigned char toggle = ppt->get("toggle", false) ? 1 : 0; \
            conf.switch_map.insert(std::make_pair(KEY_##num, switch_state{{*cc, toggle, 0}})); \
        } \
        else \
            cout << "warning: no cc assigned to switch " << num << endl; \
    } \
    else \
        cout << "warning: no switch were configured" << endl; \
  }

    AXE_SW_CONF(1)
    AXE_SW_CONF(2)
    AXE_SW_CONF(3)
    AXE_SW_CONF(4)
    AXE_SW_CONF(5)
    AXE_SW_CONF(6)
    AXE_SW_CONF(7)
#undef AXE_CONF

    conf.pedal_cc = pt.get("switch.pedal.cc", 0);

    return true;
}


bool find_port(config const& conf, snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    unsigned int const bits = SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE;

    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0)
        {
            if ((snd_seq_port_info_get_capability(pinfo) & (bits)) == bits
                && strcmp(snd_seq_client_info_get_name(cinfo), conf.portname.c_str()) == 0)
                   return true;
        }
    }
    return false;
}


struct seq_guard
{
    seq_guard(snd_seq_t*& seq)
        : seq(seq)
    {}

    ~seq_guard()
    {
        if (seq)
            snd_seq_close(seq);
    }

    snd_seq_t*& seq;
};


int main()
{

    config conf;
    if (!load_config(conf))
        return 1;

    int fd = open(conf.device.c_str(), O_RDONLY);
    if (fd == -1) {
        cout << "error: can't open device " << conf.device << endl;
        return 1;
    }

    snd_seq_t* seq = nullptr;
    seq_guard seq_{seq};

    int err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
    if (err < 0) {
        cout << "error: can't open alsa sequenser: " << snd_strerror(err) << endl;
        return 1;
    }

    snd_seq_port_info_t* pinfo = nullptr;
    snd_seq_port_info_alloca(&pinfo);
    if (!find_port(conf, seq, pinfo)) {
        cout << "error: can't find midi port " << conf.portname << endl;
        return 1;
    }

    int port = snd_seq_create_simple_port(seq, "axerig2",
                    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                    SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0) {
        cout << "error: can't create alsa port: " << snd_strerror(port) << endl;
        return 1;
    }

    err = snd_seq_connect_to(seq, port, snd_seq_port_info_get_client(pinfo)
                            ,snd_seq_port_info_get_port(pinfo));
    if (err < 0) {
        cout << "error: can't connect to Axe-Fx: " << snd_strerror(err) << endl;
        return 1;
    }

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, port);
    snd_seq_ev_set_dest(&ev, snd_seq_port_info_get_client(pinfo)
                       ,snd_seq_port_info_get_port(pinfo));
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);

    struct input_event ie;

    /// make this configurable
    constexpr int pedal_min = 264;
    constexpr int pedal_max = 3620;
    constexpr float ratio = 127.f/(pedal_max - pedal_min);
    int pval = 0;

    cout << "running..." << endl;

    for (;;)
    {
        if (read(fd, &ie, sizeof(ie)) != sizeof(ie))
            break;

        ev.type = SND_SEQ_EVENT_NONE;

        switch (ie.type)
        {
            case EV_KEY:
                if (ie.code >= KEY_1 && ie.code <= KEY_7)
                {
                    auto it = conf.switch_map.find(ie.code);
                    if (it != conf.switch_map.end())
                    {
                        unsigned char val = 0;
                        switch_state& state = it->second;
                        if (state[1]) { // toggle type
                            if  (ie.value) {
                                state[2] = state[2] ? 0 : 127;
                                val = state[2];
                            }
                            else
                                break;
                        } else
                            val = ie.value ? 127 : 0;
                        snd_seq_ev_set_controller(&ev, conf.channel, state[0], val);
                    }
                }
                break;
            case EV_ABS:
                if (ie.code == ABS_X && conf.pedal_cc && pval != ie.value)
                {
                    snd_seq_ev_set_controller(&ev, conf.channel, conf.pedal_cc
                                             ,(int)((ie.value - pedal_min)*ratio));
                    pval = ie.value;
                }
                break;
            default:
                break;
        }

        if (ev.type != SND_SEQ_EVENT_NONE)
            snd_seq_event_output_direct(seq, &ev);
    }

    close(fd);

    return 0;
}
