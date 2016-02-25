
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import hashlib
import re
import subprocess

def checksum_string(s):
    return hashlib.md5(s.encode('utf-8')).hexdigest()

def checksum(fd):
    """ Calculate a md5 checksum of fd file handle

    Arguments:
      fd: A file descriptor return from open() call

    Returns:
      A md5 checksum of the file

    Raises:
      CalledProcessError if the file doesn't exist
    """

    md5sum_cmd = ["md5sum", fd.name]
    output = subprocess.check_output(md5sum_cmd, universal_newlines=True).strip()
    return output.split()[0]

class ArchiveChecksums(dict):
    @classmethod
    def from_file_desc(cls, fd):
        checksum_pattern = re.compile(r"(\S+)\s+(\S+)")
        checksums = dict()

        for line in (line.decode('utf-8').strip() for line in fd if line):

            # Skip comments
            if line.startswith('#'):
                continue

            # Skip lines that do not contain the pattern we are looking for
            result = checksum_pattern.search(line)
            if result is None:
                continue

            chksum, filepath = result.groups()
            checksums[filepath] = chksum

        return cls(checksums)

