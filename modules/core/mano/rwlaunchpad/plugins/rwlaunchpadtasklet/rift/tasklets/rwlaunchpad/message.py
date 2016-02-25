
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

import logging
import time


class MessageException(Exception):
    def __init__(self, msg):
        self.msg = msg


class Message(object):
    """
    Messages are events that describe stages of the onboarding process, and
    any event that may occur during the onboarding process.
    """

    def __init__(self, level, name, text):
        self._level = level
        self._name = name
        self._text = text
        self._timestamp = time.time()

    def __repr__(self):
        return "{} {}:{}:{}".format(
                self.timestamp,
                logging._levelNames.get(self.level, self.level),
                self.name,
                self.text,
                )

    @property
    def level(self):
        return self._level

    @property
    def name(self):
        return self._name

    @property
    def text(self):
        return self._text

    @property
    def timestamp(self):
        return self._timestamp

    def log(self, logger):
        logger.log(self.level, self.text)


class WarningMessage(Message):
    """
    A warning is a message that does not prevent the onboarding process for
    continuing, but may not be the intention of the user when they initiated
    the process.
    """

    def __init__(self, name, text):
        super().__init__(logging.WARNING, name, text)


class ErrorMessage(Message):
    """
    An error message alerts the user to an event that prevent the continuation
    of the onboarding process.
    """

    def __init__(self, name, text):
        super().__init__(logging.ERROR, name, text)


class StatusMessage(Message):
    """
    A status message informs the user of an expected stage in the onboarding
    process.
    """

    def __init__(self, name, text):
        super().__init__(logging.INFO, name, text)

    def log(self, logger):
        pass


class Logger(object):
    """
    This class is used to augment a python logger class so that messages can be
    passed to it. Messages are recorded so that the uploader application can
    provide this information to the client, and the messages are also recorded
    on the server via the standard logging facilities.
    """

    def __init__(self, logger, messages):
        self._rift_logger = logger
        self._messages = messages

    @property
    def messages(self):
        return self._messages

    def message(self, msg):
        msg.log(self._rift_logger)
        self._messages.append(msg)

    def debug(self, msg):
        self._rift_logger.debug(msg)

    def info(self, msg):
        self._rift_logger.info(msg)

    def error(self, msg):
        self._rift_logger.error(msg)

    def fatal(self, msg):
        self._rift_logger.fatal(msg)

    def warn(self, msg):
        self._rift_logger.warn(msg)

    def warning(self, msg):
        self._rift_logger.warning(msg)

    def critical(self, msg):
        self._rift_logger.critical(msg)

    def exception(self, exc):
        self._rift_logger.exception(exc)


class OnboardStart(StatusMessage):
    def __init__(self):
        super().__init__("onboard-started", "onboarding process started")


class OnboardError(ErrorMessage):
    def __init__(self, msg):
        super().__init__("onboard-error", msg)


class OnboardWarning(ErrorMessage):
    def __init__(self, msg):
        super().__init__("onboard-warning", msg)


class OnboardPackageUpload(StatusMessage):
    def __init__(self):
        super().__init__("onboard-pkg-upload", "uploading package")


class OnboardImageUpload(StatusMessage):
    def __init__(self):
        super().__init__("onboard-img-upload", "uploading image")


class OnboardPackageValidation(StatusMessage):
    def __init__(self):
        super().__init__("onboard-pkg-validation", "package contents validation")


class OnboardDescriptorValidation(StatusMessage):
    def __init__(self):
        super().__init__("onboard-dsc-validation", "descriptor validation")


class OnboardDescriptorError(OnboardError):
    def __init__(self, filename):
        super().__init__("unable to onboard {}".format(filename))


class OnboardDescriptorOnboard(StatusMessage):
    def __init__(self):
        super().__init__("onboard-dsc-onboard", "onboarding descriptors")


class OnboardSuccess(StatusMessage):
    def __init__(self):
        super().__init__("onboard-success", "onboarding process successfully completed")


class OnboardFailure(StatusMessage):
    def __init__(self):
        super().__init__("onboard-failure", "onboarding process failed")


class OnboardMissingContentType(OnboardError):
    def __init__(self):
        super().__init__("missing content-type header")


class OnboardUnsupportedMediaType(OnboardError):
    def __init__(self):
        super().__init__("multipart/form-data required")


class OnboardMissingContentBoundary(OnboardError):
    def __init__(self):
        super().__init__("missing content boundary")


class OnboardMissingTerminalBoundary(OnboardError):
    def __init__(self):
        super().__init__("Unable to find terminal content boundary")


class OnboardUnreadableHeaders(OnboardError):
    def __init__(self):
        super().__init__("Unable to read message headers")


class OnboardUnreadablePackage(OnboardError):
    def __init__(self):
        super().__init__("Unable to read package")


class OnboardMissingChecksumsFile(OnboardError):
    def __init__(self):
        super().__init__("Package does not contain checksums.txt")


class OnboardChecksumMismatch(OnboardError):
    def __init__(self, filename):
        super().__init__("checksum mismatch for {}".format(filename))


class OnboardMissingAccount(OnboardError):
    def __init__(self):
        super().__init__("no account information available")


class OnboardMissingFile(OnboardWarning):
    def __init__(self, filename):
        super().__init__("{} is not in the archive".format(filename))


class OnboardInvalidPath(OnboardWarning):
    def __init__(self, filename):
        super().__init__("{} is not a valid package path".format(filename))


class ExportStart(StatusMessage):
    def __init__(self):
        super().__init__("export-started", "export process started")


class ExportSuccess(StatusMessage):
    def __init__(self):
        super().__init__("export-success", "export process successfully completed")


class ExportFailure(StatusMessage):
    def __init__(self):
        super().__init__("export-failure", "export process failed")




class UpdateError(ErrorMessage):
    def __init__(self, msg):
        super().__init__("update-error", msg)


class UpdateMissingAccount(UpdateError):
    def __init__(self):
        super().__init__("no account information available")

class UpdateMissingContentType(UpdateError):
    def __init__(self):
        super().__init__("missing content-type header")


class UpdateUnsupportedMediaType(UpdateError):
    def __init__(self):
        super().__init__("multipart/form-data required")


class UpdateMissingContentBoundary(UpdateError):
    def __init__(self):
        super().__init__("missing content boundary")


class UpdateStart(StatusMessage):
    def __init__(self):
        super().__init__("update-started", "update process started")


class UpdateSuccess(StatusMessage):
    def __init__(self):
        super().__init__("update-success", "updating process successfully completed")


class UpdateFailure(StatusMessage):
    def __init__(self):
        super().__init__("update-failure", "updating process failed")


class UpdatePackageUpload(StatusMessage):
    def __init__(self):
        super().__init__("update-pkg-upload", "uploading package")


class UpdateDescriptorError(UpdateError):
    def __init__(self, filename):
        super().__init__("unable to update {}".format(filename))


class UpdateDescriptorUpdated(StatusMessage):
    def __init__(self):
        super().__init__("update-dsc-updated", "updated descriptors")


class UpdateUnreadableHeaders(UpdateError):
    def __init__(self):
        super().__init__("Unable to read message headers")


class UpdateUnreadablePackage(UpdateError):
    def __init__(self):
        super().__init__("Unable to read package")


class UpdateChecksumMismatch(UpdateError):
    def __init__(self, filename):
        super().__init__("checksum mismatch for {}".format(filename))


class UpdateNewDescriptor(UpdateError):
    def __init__(self, filename):
        super().__init__("{} contains a new descriptor".format(filename))
