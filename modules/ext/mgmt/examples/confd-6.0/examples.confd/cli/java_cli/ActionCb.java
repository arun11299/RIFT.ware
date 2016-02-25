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

import com.tailf.dp.*;
import com.tailf.dp.proto.*;
import com.tailf.dp.annotations.*;
import com.tailf.conf.*;
import com.tailf.maapi.*;
import java.io.*;
import java.net.*;

/**
 * This is our action callback
 *
 */
public class ActionCb {


    @ActionCallback(callPoint="cli-point",
                      callType={ActionCBType.INIT})
    public void init(DpActionTrans trans) throws DpCallbackException {
        trace("init(): userinfo= "+trans.getUserInfo());
    }

    /**
     * This is the action callback.
     * In this example we have single function
     * for all three actions.
     */
    @ActionCallback(callPoint="cli-point",
                      callType={ActionCBType.ACTION})
    public ConfXMLParam[] action(DpActionTrans trans, ConfTag name,
                                    ConfObject[] kp, ConfXMLParam[] params)
        throws DpCallbackException {

        try {
            trace("action callback called!");

            for (int i=0;i<params.length;i++) {
                trace("param: "+i+": "+params[i]);
            }

            trace("connecting via Maapi");
            Socket maapi_socket= new Socket("127.0.0.1",Conf.PORT);
            Maapi maapi = new Maapi(maapi_socket);

            int usid = trans.getUserInfo().getUserId();

            maapi.CLIWrite(usid,"Running CLI command\n");
            maapi.CLIWrite(usid,"You supplied "+params.length+" arguments\n");

            String[] choice= new String[] { "yes", "no" };
            String res = maapi.CLIPromptOneOf(usid,"Do you want to proceed: ",choice);

            if (res.equals("yes"))  {
                maapi.CLIWrite(usid,"Proceeding...\n");
                res = maapi.CLIPrompt(usid,"Enter password please: ",true);
            }
            else
                maapi.CLIWrite(usid,"Not proceeding...\n");
            return null;
        } catch (Exception e) {
            throw new DpCallbackException("action: got: "+e);
        }
    }


    /**
     * This is a commandpoint.
     */
    @ActionCallback(callPoint="cli-point",
                      callType={ActionCBType.COMMAND})
    public String[] command(DpActionTrans trans, String cmdname,
                            String cmdpath, String[] params)
        throws DpCallbackException {
        try {
            trace("command callback called!");

            for (int i=0;i<params.length;i++) {
                trace("param: "+i+": "+params[i]);
            }

            trace("connecting via Maapi");
            Socket maapi_socket= new Socket("127.0.0.1",Conf.PORT);
            Maapi maapi = new Maapi(maapi_socket);

            int usid = trans.getUserInfo().getUserId();

            maapi.CLIWrite(usid,"Running CLI command\n");
            maapi.CLIWrite(usid,"You supplied "+params.length+" arguments\n");

            String[] choice= new String[] { "yes", "no" };
            String res = maapi.CLIPromptOneOf(usid,"Do you want to proceed: ",choice);

            if (res.equals("yes")) {
                maapi.CLIWrite(usid,"Proceeding...\n");
                res = maapi.CLIPrompt(usid,"Enter password please: ",true);
            }
            else
                maapi.CLIWrite(usid,"Not proceeding...\n");
            return new String[] { res };

        } catch (Exception e) {
            throw new DpCallbackException("action: got: "+e);
        }
    }


    private void trace(String str) {
        System.err.println("*ActionCb: "+str);
    }
}

