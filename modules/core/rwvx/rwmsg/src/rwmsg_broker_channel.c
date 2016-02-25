
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwmsg_broker_channel.c
 * @author Grant Taylor <grant.taylor@riftio.com>
 * @date 2014/01/26
 * @brief RWMSG broker base channel
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "rwmsg_int.h"
#include "rwmsg_broker.h"

void rwmsg_broker_channel_create(rwmsg_broker_channel_t *bch,
				 enum rwmsg_chantype_e typ,
				 rwmsg_broker_t *bro,
				 struct rwmsg_broker_channel_acceptor_key_s *key) {
  rwmsg_channel_create(&bch->ch, bro->ep, typ);

  bch->ctime = time(NULL);
  bch->locid = (uint32_t)(bch->ctime & 0x00ffffff);
  bch->id.chanid = bro->chanid_nxt;
  bch->bro = bro;

  RW_ASSERT(bch->id.chanid);
  if (bro->chanid_nxt < UINT32_MAX) {
    bro->chanid_nxt++;
  } else {
    /* We don't avoid collisions with ancient channels after wrap */ 
    RW_ASSERT(0);
    abort();
  }

  memcpy(&bch->acc_key, key, sizeof(bch->acc_key));
  HASH_ADD(hh, bro->acc.bch_hash, acc_key, sizeof(bch->acc_key), bch);


  if (bro->use_mainq) {
    /* Bind all channels to main queue */
    bch->rwq = rwsched_dispatch_get_main_queue(bro->ep->rwsched);
  } else {
    /* Bind each channel to a serial queue */
    bch->rwq = rwsched_dispatch_queue_create(bro->ep->taskletinfo,
                                             typ == RWMSG_CHAN_BROSRV ? "bsrvchan"
                                             :typ == RWMSG_CHAN_BROCLI ?  "bclichan"
                                             :typ == RWMSG_CHAN_PEERSRV ? "psrvchan"
                                             :typ == RWMSG_CHAN_PEERCLI ? "pclichan"
                                             : "duh?", DISPATCH_QUEUE_SERIAL);
  }
  rwmsg_broker_g.exitnow.bch_count++;
}

void rwmsg_broker_channel_destroy(rwmsg_broker_channel_t *bch) {
  if (bch->rwq && bch->rwq != rwsched_dispatch_get_main_queue(bch->bro->ep->rwsched)) {
    //defer via gc mechanism: rwsched_dispatch_release(bch->bro->ep->rwsched, bch->bro->ep->taskletinfo, bch->q);
    rwmsg_garbage(&bch->bro->ep->gc, RWMSG_OBJTYPE_RWSCHED_OBJREL, bch->rwq, bch->bro->ep->rwsched, bch->bro->ep->taskletinfo);
  }
  bch->rwq = NULL;
  rwmsg_broker_g.exitnow.bch_count--;
}
