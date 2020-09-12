<template>
  <b-card
    :header="$t('title')"
    header-tag="h6"
    header-bg-variant="secondary"
    header-text-variant="white"
    class="mb-3"
  >
    <b-form @submit.stop.prevent>
      <b-form-group :label="$t('serial')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.serial" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('cpuUsage')" label-cols-sm="4">
        <b-progress max="100" height="2.25rem" class="form-control p-0">
          <b-progress-bar
            :value="this.$store.state.sysInfo.cpuUsage"
            :label-html="`<span class='justify-content-center d-flex position-absolute w-100 text-dark'>${this.$store.state.sysInfo.cpuUsage.toFixed(2)}%</span>`"
          ></b-progress-bar>
        </b-progress>
      </b-form-group>
      <b-form-group :label="$t('memoryUsage')" label-cols-sm="4">
        <b-progress max="100" height="2.25rem" class="form-control p-0">
          <b-progress-bar
            :value="this.$store.state.sysInfo.memoryUsage"
            :label-html="`<span class='justify-content-center d-flex position-absolute w-100 text-dark'>${this.$store.state.sysInfo.memoryUsage.toFixed(2)}%</span>`"
          ></b-progress-bar>
        </b-progress>
      </b-form-group>
      <b-form-group :label="$t('rawUartRemoteAddress')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.rawUartRemoteAddress" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('radioModuleType')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.radioModuleType" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('radioModuleSerial')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.radioModuleSerial" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('radioModuleBidCosRadioMAC')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.radioModuleBidCosRadioMAC" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('radioModuleHmIPRadioMAC')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.radioModuleHmIPRadioMAC" disabled></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('radioModuleSGTIN')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.radioModuleSGTIN" disabled></b-form-input>
      </b-form-group>
    </b-form>
  </b-card>
</template>

<script>
import Vue from "vue";

import VueI18n from "vue-i18n";
Vue.use(VueI18n);

import { LayoutPlugin } from "bootstrap-vue/esm/components/layout";
Vue.use(LayoutPlugin);

import { FormPlugin } from "bootstrap-vue/esm/components/form";
Vue.use(FormPlugin);

import { FormGroupPlugin } from "bootstrap-vue/esm/components/form-group";
Vue.use(FormGroupPlugin);

import { FormInputPlugin } from "bootstrap-vue/esm/components/form-input";
Vue.use(FormInputPlugin);

import { ProgressPlugin } from "bootstrap-vue/esm/components/progress";
Vue.use(ProgressPlugin);

import { CardPlugin } from "bootstrap-vue/esm/components/card";
Vue.use(CardPlugin);

export default {
  name: "SysInfo",
  data() {
    return {};
  },
  created() {
    var self = this;
    this.updateTimer = setInterval(() => {
      self.$store.dispatch("sysInfo/update");
    }, 1000);
  },
  beforeDestroy() {
    clearInterval(this.updateTimer);
  },
  i18n: {
    locale: navigator.language,
    fallbackLocale: "en",
    messages: {
      de: {
        title: "Systeminformationen",
        serial: "Seriennummer",
        cpuUsage: "CPU Auslastung",
        memoryUsage: "Speicherauslastung",
        rawUartRemoteAddress: "Verbunden mit",
        radioModuleType: "Funkmodultyp",
        radioModuleSerial: "Seriennummer",
        radioModuleBidCosRadioMAC: "Funkadresse (BidCos)",
        radioModuleHmIPRadioMAC: "Funkadresse (HmIP)",
        radioModuleSGTIN: "SGTIN"
      },
      en: {
        title: "System information",
        serial: "Serial number",
        cpuUsage: "CPU usage",
        memoryUsage: "Memory usage",
        rawUartRemoteAddress: "Connected with",
        radioModuleType: "Radio module type",
        radioModuleSerial: "Serial number",
        radioModuleBidCosRadioMAC: "Radio address (BidCos)",
        radioModuleHmIPRadioMAC: "Radio address (HmIP)",
        radioModuleSGTIN: "SGTIN"
      }
    }
  }
};
</script>

<style lang="css">
</style>