/*    -*- Java -*-
 *
 *  Copyright 2007 Tail-F Systems AB. All rights reserved.
 *
 *  This software is the confidential and proprietary
 *  information of Tail-F Systems AB.
 *
 *  $Id$
 *
 */

import com.tailf.conf.*;
import com.tailf.dp.*;
import com.tailf.maapi.*;
import com.tailf.dp.annotations.*;
import com.tailf.dp.proto.*;
import java.io.*;
import java.net.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Calendar;


public class Demo {

    /** ------------------------------------------------------------
     *   main
     *
     */
    static public void main(String args[]) throws Exception {


        /* create maapi instance to be used by the  validate callbacks */
        Maapi maapi = new Maapi( new Socket("localhost", Conf.PORT) );

        // create new control socket
        Socket ctrlSocket = new Socket("127.0.0.1", Conf.PORT);

        // init and connect control socket
        Dp dp = new Dp("arpstats", ctrlSocket);

        // register the stats callbacks
        dp.registerAnnotatedCallbacks( new StatsTrans() );
        dp.registerAnnotatedCallbacks( new StatsCb() );

        dp.registerDone();
        System.out.println("demo_started");
        // read input from the control socket
        try {
            while (true) dp.read();
        }
        catch (Exception e) {
            System.out.println("ConfD terminated");
        }

    }

    public static class StatsCb {


        private ArrayList entries = new ArrayList();
        private long lastPop = -1;

        private void populateEntries() {
            // refresh cache if its older than 3 secs
            long now = Calendar.getInstance().getTimeInMillis() / 1000;
            if (lastPop == -1)
                lastPop = now;
            else if (lastPop + 3 > now)
                return;
            else
                lastPop = now;
            entries = new ArrayList();
            try {
                String s;
                Process p = Runtime.getRuntime().exec("arp -an");

                BufferedReader stdInput = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

                BufferedReader stdError = new BufferedReader(
                    new InputStreamReader(p.getErrorStream()));

                // read the output from the command

                while ((s = stdInput.readLine()) != null) {
                    String[] toks = s.split(" ");
                    if (toks.length < 4)
                        continue;
                    ArpEntry e = new ArpEntry();
                    String ss = toks[1];
                    e.ip = ss.substring(
                        1, ss.length()).substring(0, ss.length()-2);
                    e.hwaddr = toks[3];
                    if (e.hwaddr.compareTo("<incomplete>") == 0)
                        continue;
                    e.ifname = toks[toks.length-1];
                    e.permanent = false; e.published = false;
                    for (int i= 4; i<toks.length; i++) {
                        if (toks[i].compareTo("PERM") == 0)
                            e.permanent = true;
                        if (toks[i].compareTo("PUB") == 0)
                            e.published = true;
                    }
                    entries.add(e);
                }
            }
            catch (IOException e) {
                System.out.println("exception happened - here's what I know: ");
                e.printStackTrace();
            }
        }

        @DataCallback(callPoint = jdemo.callpoint_jarp_data,
                      callType = { DataCBType.ITERATOR })
        public Iterator<Object> iterator(DpTrans trans, ConfObject[] kp)
            throws DpCallbackException {
                populateEntries();
            System.out.println("<<<<<<: "+entries.size());
            return entries.iterator();
        }

        @DataCallback(callPoint = jdemo.callpoint_jarp_data,
                      callType = { DataCBType.GET_NEXT })
        public ConfKey getKey(DpTrans trans, ConfObject[] kp, Object obj)
            throws DpCallbackException {
            ArpEntry e = (ArpEntry) obj;
            ConfObject c;
            try {
                c =  new ConfIPv4(e.ip);
            }
            catch (ConfException e2) {
                throw new DpCallbackException(e2.getMessage());
            }
            return new ConfKey( new ConfObject[] { c, new ConfBuf(e.ifname)});
        }

        @DataCallback(callPoint =jdemo.callpoint_jarp_data,
                      callType = { DataCBType.GET_ELEM })
        public ConfValue getElem(DpTrans trans, ConfObject[] kp)  {
            String ip = ((ConfKey) kp[1]).elementAt(0).toString();
            String ifname = ((ConfKey) kp[1]).elementAt(1).toString();
            for (int i=0; i<entries.size(); i++) {
                ArpEntry e = (ArpEntry)entries.get(i);
                if ((e.ip.compareTo(ip) == 0)  &&
                    (e.ifname.compareTo(ifname) == 0)) {
                    ConfTag leaf = (ConfTag) kp[0];
                    switch (leaf.getTagHash()) {
                    case jdemo.jdemo_ip:
                        try {
                            ConfObject c =  new ConfIPv4(e.ip);
                        }
                        catch (ConfException e2) {
                            return null;
                        }
                    case jdemo.jdemo_hwaddr:
                        return new ConfBuf(e.hwaddr);
                    case jdemo.jdemo_permanent:
                        return new ConfBool(e.permanent);
                    case jdemo.jdemo_published:
                        return new ConfBool(e.published);
                    default:
                        return  null;
                    }
                }
            }
            return null; /* not found */
        }

        private class ArpEntry {
            String ip;
            String hwaddr;
            boolean permanent;
            boolean published;
            String ifname;
        }
    }

    public static class StatsTrans {
        @TransCallback(callType = { TransCBType.INIT })
        public void init(DpTrans trans) {
        }
    }
}
