namespace RwMon {
  public interface Monitoring: GLib.Object {
    /**
     * Init routine
     *
     * @param log_ctx - [in] the log context to use
     *
     * @return RwStatus
     */
    public abstract RwTypes.RwStatus init(RwLog.Ctx log_ctx);

    /**
     * nfvi_metrics
     *
     * Returns the NFVI metrics for a particular VM
     *
     * @param account - [in] the account details of the owner of the VM
     * @param vm_id   - [in] the ID of the VM to retrieve the metrics from
     * @param metrics - [out] An NfviMetrics object
     *
     * @return RwStatus
     */
    public abstract RwTypes.RwStatus nfvi_metrics(
      Rwcal.CloudAccount account,
      string vm_id,
      out Rwmon.NfviMetrics metrics);

    /**
     * nfvi_metrics_available
     *
     * Checks whether ceilometer exists for this account and is 
     * providing NFVI metrics
     *
     * @param account - [in] the account details of the owner of the VM
     * @param present - [out] True if ceilometer exists, False otherwise
     *
     * @return RwStatus
     */
    public abstract RwTypes.RwStatus nfvi_metrics_available(
      Rwcal.CloudAccount account,
      out bool present);
  }
}
