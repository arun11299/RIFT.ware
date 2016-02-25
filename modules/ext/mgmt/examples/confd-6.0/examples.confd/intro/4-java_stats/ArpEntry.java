
import com.tailf.conf.ConfIPv4;
import com.tailf.conf.ConfException;
public class ArpEntry implements Comparable<ArpEntry>{
    private ConfIPv4 ip;
    private String hwaddr;
    private boolean permanent;
    private boolean published;
    private String ifname;
    //Compares its two arguments for order.

    public ArpEntry(String ip,String hwaddr,boolean permanent,
                    boolean published,String ifname){
        try{
            this.ip = new ConfIPv4(ip);
        }catch(ConfException e){          }
        this.hwaddr = hwaddr;
        this.permanent = permanent;
        this.published = published;
        this.ifname = ifname;
    }

    public ConfIPv4 getIPAddress(){
        return this.ip;
    }

    public boolean getPermanent(){
        return this.permanent;
    }

    public boolean getPublished(){
        return this.published;
    }
    public String getHWAddress(){
        return this.hwaddr;
    }

    public String getIfName(){
        return this.ifname;

    }
    public int compareTo(ArpEntry arpe) {
        int cmp = 0;

        if((cmp = ip.compareTo(arpe.getIPAddress())) != 0)
            return cmp;
        else
            return arpe.ifname.compareTo(arpe.getIfName());
    }
    //Indicates whether some other object is "equal to" this comparator.
    public boolean equals(Object obj) {
        if(obj.getClass() != this.getClass())
            return false;

        ArpEntry arpEntry = (ArpEntry)obj;

        return this.ip.equals(arpEntry.getIPAddress()) &&
            this.ifname.equals(arpEntry.getIfName());

    }
}