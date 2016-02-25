;; Add this directory to Emacs' load path
(add-to-list 'load-path "~/.emacs.d")

;; load google style
(require 'google-c-style)              ; Load google-c-style
(add-hook 'c-mode-common-hook 'google-set-c-style)
(add-hook 'c-mode-common-hook 'google-make-newline-indent)

;; load yang mode
(autoload 'yang-mode "yang-mode" "Major mode for editing YANG modules." t)
(add-to-list 'auto-mode-alist '("\\.yang$" . yang-mode))
(defun google-yang-mode-hook ()
  "Configuration for YANG Mode. Add this to `yang-mode-hook'."
  (c-set-style "google-c-style")
  (setq c-basic-offset 2))
(add-hook 'yang-mode-hook 'google-yang-mode-hook)


;; load cmake mode
;; (setq load-path (cons (expand-file-name "/home/agunturu/.emacs.d/cmake-mode") load-path))
(require 'cmake-mode)
(setq auto-mode-alist
  (append '(("CMakeLists\\.txt\\'" . cmake-mode)
  ("\\.cmake\\'" . cmake-mode)
  ("\\.cmake\\.in\\'" . cmake-mode))
  auto-mode-alist))

