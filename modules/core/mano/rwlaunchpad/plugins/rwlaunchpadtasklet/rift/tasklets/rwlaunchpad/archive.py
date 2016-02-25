
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import os
import re
import shutil
import tempfile

from . import checksums
from . import convert
from . import message


class ArchiveError(Exception):
    pass


class ArchiveInvalidPath(message.ErrorMessage):
    def __init__(self, filename):
        msg =  "unable to match checksum filename {} to contents of archive"
        super().__init__("archive-error", msg.format(filename))


class LaunchpadArchive(object):
    def __init__(self, tar, log):
        self._descriptors = dict()
        self._descriptors['images'] = list()
        self._descriptors['pnfd'] = list()
        self._descriptors['vnfd'] = list()
        self._descriptors['vld'] = list()
        self._descriptors['nsd'] = list()
        self._descriptors['vnffgd'] = list()
        self._descriptors['schema/libs'] = list()
        self._descriptors['schema/yang'] = list()
        self._descriptors['schema/fxs'] = list()

        self._checksums = dict()
        self._manifest = None

        self.log = log
        self.tarfile = tar
        self.prefix = os.path.commonprefix(self.tarfile.getnames())

        # There must be a checksums.txt file in the archive
        if os.path.join(self.prefix, 'checksums.txt') not in tar.getnames():
            self.log.message(message.OnboardMissingChecksumsFile())
            raise ArchiveError()

        # Iterate through the paths in the checksums files and validate them.
        # Note that any file in the archive that is not included in the
        # checksums file will be ignored.
        fd = tar.extractfile(os.path.join(self.prefix, 'checksums.txt'))
        archive_checksums = checksums.ArchiveChecksums.from_file_desc(fd)

        def validate_checksums():
            archive_files = {info.name for info in self.tarfile.getmembers() if info.isfile()}

            # Identify files in the checksums.txt file that cannot be located in
            # the archive.
            for filename in archive_checksums:
                if os.path.join(self.prefix, filename) not in archive_files:
                    self.log.message(message.OnboardMissingFile(filename))
                    raise ArchiveError()

            # Use the checksums to validate the remaining files in the archive
            for filename in archive_checksums:
                path = os.path.join(self.prefix, filename)
                if checksums.checksum(self.tarfile.extractfile(path)) != archive_checksums[filename]:
                    self.log.message(message.OnboardChecksumMismatch(filename))
                    raise ArchiveError()

        # Disable checksum validations for onboard performance issues
        # validate_checksums()

        def assign_manifest(filename):
            self._manifest = filename

        patterns = [
                (re.compile(r"images/([^/]+)"), self._descriptors["images"].append),
                (re.compile(r"pnfd/([^/]+)"), self._descriptors["pnfd"].append),
                (re.compile(r"vnfd/([^/]+)"), self._descriptors["vnfd"].append),
                (re.compile(r"vld/([^/]+)"), self._descriptors["vld"].append),
                (re.compile(r"nsd/([^/]+)"), self._descriptors["nsd"].append),
                (re.compile(r"vnffgd/([^/]+)"), self._descriptors["vnffgd"].append),
                (re.compile(r"schema/libs/([^/]+)"), self._descriptors["schema/libs"].append),
                (re.compile(r"schema/yang/([^/]+)"), self._descriptors["schema/yang"].append),
                (re.compile(r"schema/fxs/([^/]+)"), self._descriptors["schema/fxs"].append),
                (re.compile(r"manifest.xml"), assign_manifest),
                ]

        # Iterate through the recognized patterns and assign files accordingly
        for filename in archive_checksums:
            relname = os.path.relpath(filename)
            for pattern, store in patterns:
                if pattern.match(relname):
                    store(relname)
                    self._checksums[relname] = archive_checksums[filename]
                    break

            else:
                raise message.MessageException(ArchiveInvalidPath(filename))

    @property
    def checksums(self):
        """A dictionary of the file checksums"""
        return self._checksums

    @property
    def pnfds(self):
        """A list of PNFDs in the archive"""
        return self._descriptors['pnfd']

    @property
    def vnfds(self):
        """A list of VNFDs in the archive"""
        return self._descriptors['vnfd']

    @property
    def vlds(self):
        """A list of VLDs in the archive"""
        return self._descriptors['vld']

    @property
    def vnffgds(self):
        """A list of VNFFGDs in the archive"""
        return self._descriptors['vnffgd']

    @property
    def nsds(self):
        """A list of NSDs in the archive"""
        return self._descriptors['nsd']

    @property
    def images(self):
        """A list of images in the archive"""
        return self._descriptors['images']

    @property
    def filenames(self):
        """A list of all the files in the archive"""
        return self.pnfds + self.vnfds + self.vlds + self.vnffgds + self.nsds + self.images

    def extract(self, dest):
        # Ensure that the destination directory exists
        if not os.path.exists(dest):
            os.makedirs(dest)

        for filename in self.filenames:
            # Create the full name to perform the lookup for the TarInfo in the
            # archive.
            fullname = os.path.join(self.prefix, filename)
            member = self.tarfile.getmember(fullname)

            # Make sure that any preceeding directories in the path have been
            # created.
            dirname = os.path.dirname(filename)
            if not os.path.exists(os.path.join(dest, dirname)):
                os.makedirs(os.path.join(dest, dirname))

            # Copy the contents of the file to the correct path
            with open(os.path.join(dest, filename), 'wb') as dst:
                src = self.tarfile.extractfile(member)
                shutil.copyfileobj(src, dst, 10 * 1024 * 1024)
                src.close()

class PackageArchive(object):
    def __init__(self):
        self.images = dict()
        self.vnfds = list()
        self.nsds = list()
        self.vlds = list()
        self.checksums = dict()

    def add_image(self, image, chksum=None):
        if image.name not in self.images:
            if chksum is None:
                with open(image.location, 'r+b') as fp:
                    self.checksums["images/" + image.name] = checksums.checksum(fp)

            else:
                self.checksums["images/" + image.name] = chksum

            self.images[image.name] = image

    def add_vld(self, vld):
        self.vlds.append(vld)

    def add_vnfd(self, vnfd):
        self.vnfds.append(vnfd)

    def add_nsd(self, nsd):
        self.nsds.append(nsd)

    def create_archive(self, archive_name, dest=None):
        if dest is None:
            dest = tempfile.gettempdir()

        if archive_name.endswith(".tar.gz"):
            archive_name = archive_name[:-7]

        archive_path = os.path.join(dest, archive_name)

        if os.path.exists(archive_path):
            shutil.rmtree(archive_path)

        os.makedirs(archive_path)

        def write_descriptors(descriptors, converter, name):
            if descriptors:
                os.makedirs(os.path.join(archive_path, name))

                path = "{}/{{}}.xml".format(os.path.join(archive_path, name))
                for desc in descriptors:
                    xml = converter.to_xml_string(desc)
                    open(path.format(desc.id), 'w').write(xml)

                    key = os.path.relpath(path.format(desc.id), archive_path)
                    self.checksums[key] = checksums.checksum_string(xml)

        def write_images():
            if self.images:
                image_path = os.path.join(archive_path, "images")
                os.makedirs(image_path)

                for image in self.images.values():
                    shutil.copy2(image.location, image_path)

        def write_checksums():
            with open(os.path.join(archive_path, "checksums.txt"), "w") as fp:
                for path, chksum in self.checksums.items():
                    fp.write("{} {}\n".format(chksum, path))

        # Start by writing the descriptors to the archive
        write_descriptors(self.nsds, convert.NsdYangConverter(), "nsd")
        write_descriptors(self.vlds, convert.VldYangConverter(), "vld")
        write_descriptors(self.vnfds, convert.VnfdYangConverter(), "vnfd")

        # Copy the images to the archive
        write_images()

        # Finally, write the checksums file
        write_checksums()

        # Construct a tarball
        cmd = "tar zcf {dest}/{name}.tar.gz.partial -C {dest} {name} &>/dev/null"
        os.system(cmd.format(name=archive_name, dest=dest))

        # Rename to final name
        cmd = "mv {dest}/{name}.tar.gz.partial {dest}/{name}.tar.gz"
        os.system(cmd.format(name=archive_name, dest=dest))

        shutil.rmtree(archive_path)
