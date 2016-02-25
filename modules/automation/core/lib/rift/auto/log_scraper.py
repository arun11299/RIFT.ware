"""
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

@file log_scraper.py
@date 12/14/2015
@brief Simple scraper for scraping remote logs using ssh and tail
"""

import subprocess
import logging

logger = logging.getLogger(__name__)

class RemoteLogScraper(object):
    '''Scraper for grabbing data from remote logs via ssh'''

    def __init__(self, abspath):
        '''Initializes a RemoteLogScraper that scrapes the resource located at abspath

        Arguments:
            abspath - absolute path to resource being scraped
        '''
        self.lines_scraped = 1
        self.path = abspath
        self.host = None

    def connect(self, host):
        '''Associate scraper with the specified host'''
        self.host = host

    def connected(self):
        '''Return whether or not the scraper has been assigned a host'''
        return self.host is not None

    def scrape(self, discard=False):
        '''Retrieve new content from the resource being scraped

        Arguments:
            discard - don't decode and return data scraped from resource
        '''
        if self.host is None:
            return 'Skipped scrape - not connected'

        tail_cmd = ('ssh -o BatchMode=yes -o StrictHostKeyChecking=no -- {host} '
                    '"tail -n +{lines_scraped} {path}"').format(
                        host=self.host,
                        lines_scraped=self.lines_scraped,
                        path=self.path
                    )

        try :
            scraped = subprocess.check_output(tail_cmd, shell=True)
        except subprocess.CalledProcessError:
            logger.info("Failed to scrape %s:%s", self.host, self.path)
            return ""

        self.lines_scraped += len(scraped.splitlines())

        if discard:
            return ""

        return scraped.decode('utf-8')

    def reset(self):
        '''seek back to the beginning of the resource, should be used in the case
        that the targeted resource is expected to be truncated or if the resource
        being targeted behaves like a stream rather than a file
        '''
        self.lines_scraped = 1

