#!/usr/bin/env python

import h5py
import logging
import numpy as np
import pathlib
from matplotlib import pyplot as plt
from matplotlib.colors import LogNorm

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def plot_set_file(stat_filename: pathlib.Path) -> None:

  with h5py.File(stat_filename, "r") as f:

      logger.debug(f"Keys in stat file: {f.keys()}")

      mean_key = "MEAN_FREQUENCY_AVG"
      mean_fs_obj = f[mean_key]
      mean_arr = mean_fs_obj[()]
      logger.info(f"Means [pol,dim]={mean_arr.flatten()}")

      var_key = "VARIANCE_FREQUENCY_AVG"
      var_fs_obj = f[var_key]
      var_arr = np.sqrt(var_fs_obj[()])
      logger.info(f"Stddev [pol,dim]={var_arr.flatten()}")

      sg_key = "SPECTROGRAM"
      sg_ds_obj = f[sg_key]  # returns a h5py dataset object
      sg_arr = sg_ds_obj[()]  # returns a numpy array
      logger.info(f"Shape of {sg_key} array: {sg_arr.shape}")
      ntime = sg_arr.shape[1]
      nfreq = sg_arr.shape[2]

      ts_key = "TIMESERIES"
      ts_ds_obj = f[ts_key]  # returns a h5py dataset object
      ts_arr = ts_ds_obj[()]  # returns a numpy array
      logger.info(f"Shape of {ts_key} array: {ts_arr.shape}")
      ntime = ts_arr.shape[1]
      ndim = ts_arr.shape[2]
      mean0 = ts_arr[0].transpose()
      mean1 = ts_arr[1].transpose()

      bp_key = "MEAN_SPECTRAL_POWER"
      bp_ds_obj = f[bp_key]
      bp_arr = bp_ds_obj[()].transpose()
      logger.info(f"Shape of {bp_key} array: {bp_arr.shape}")

      hist_key = "HISTOGRAM_1D_FREQ_AVG"
      hist_ds_obj = f[hist_key]  # returns a h5py dataset object
      hist_arr = hist_ds_obj[()]  # returns a numpy array
      logger.info(f"Shape of {hist_key} array: {hist_arr.shape}")  # pylint: disable=no-member
      nbin = hist_arr.shape[2]  # pylint: disable=no-member
      xbins = np.arange(-nbin/2, nbin/2)

      # need to find the first non zero value
      sbins = np.zeros((2,2))
      ebins = np.zeros((2,2))
      sbins.fill(nbin)
      ebins.fill(nbin)
      for i in range(2):
          for j in range(2):
              for k in range(nbin):
                  if sbins[i][j] == nbin and hist_arr[i][j][k] > 0:
                      sbins[i][j] = k
              for k in range(nbin):
                  if ebins[i][j] == nbin and hist_arr[i][j][(nbin-1)-k] > 0:
                      ebins[i][j] = (nbin-1)-k
      ibin = np.amin(sbins)
      jbin = np.amax(ebins)

      ax = plt.GridSpec(4, 1)
      ax.update(wspace=0.1, hspace=0.3)

      ax1 = plt.subplot(ax[0, 0])
      ax2 = plt.subplot(ax[1, 0])
      ax3 = plt.subplot(ax[2, 0])
      ax4 = plt.subplot(ax[3, 0])

      mean = np.mean(sg_arr[0])
      stddev = np.std(sg_arr[0])
      minval = mean - 1 * stddev
      maxval = mean + 1 * stddev
      im = ax1.imshow(sg_arr[0], vmin=minval, vmax=maxval, aspect='auto')
      ax1.set_ylabel("Channel")

      ax2.plot(mean0[2], label='polA')
      ax2.plot(mean1[2], label='polB')
      ax2.set_ylabel("Power")
      ax2.set_xlabel("Time Sample")

      ax3.plot(bp_arr, label=['polA', 'polB'])
      ax3.set_xlabel("Channel")
      ax3.set_ylabel("Power")

      ax4.set_xlim([xbins[int(ibin)], xbins[int(jbin)]])
      ax4.plot(xbins, hist_arr[0][0], label='polA')
      ax4.plot(xbins, hist_arr[1][0], label='polB')
      ax4.set_xlabel("Input state")
      ax4.set_ylabel("Counts")
      ax4.legend()

      plt.colorbar(im)
      plt.show()


if __name__ == "__main__":

    import argparse
    p = argparse.ArgumentParser()
    p.add_argument(
        "stat_filename",
        type=pathlib.Path,
        help="PST Statistics File in HDF5 format.",
    )
    args = p.parse_args()
    plot_set_file(args.stat_filename)
