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
import com.tailf.dp.proto.*;
import com.tailf.dp.annotations.*;
import java.io.*;
import java.net.*;


public class Actions {

    /** ------------------------------------------------------------
     *   main
     *
     */
    static public void main(String args[]) {

        try {

            /* create new control socket */
            Socket ctrl_socket= new Socket("127.0.0.1",Conf.PORT);

            /* init and connect the control socket */
            Dp dp = new Dp("cli_actions_daemon",ctrl_socket);

            /* register the callbacks */
            dp.registerAnnotatedCallbacks( new ActionCb() );
            dp.registerDone();

            System.out.println("Actions started");
            /* read input from control socket */
            while (true) {
                dp.read();
            }

        } catch (Exception e) {
            System.err.println("(closing) "+e.getMessage());
        }
    }
}