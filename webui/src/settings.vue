<template>
  <b-card
    :header="$t('title')"
    header-tag="h6"
    header-bg-variant="secondary"
    header-text-variant="white"
    class="mb-3"
  >
    <b-form @submit.stop.prevent>
      <b-form-group :label="$t('changePassword')" label-cols-sm="4">
        <b-form-input
          type="password"
          v-model="$v.adminPassword.$model"
          :state="validateState('adminPassword')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('repeatPassword')" label-cols-sm="4">
        <b-form-input
          type="password"
          v-model="$v.adminPasswordRepeat.$model"
          :state="validateState('adminPasswordRepeat')"
        ></b-form-input>
      </b-form-group>
      <hr />
      <b-form-group :label="$t('hostname')" label-cols-sm="4">
        <b-form-input
          type="text"
          v-model="$v.hostname.$model"
          trim
          :state="validateState('hostname')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('dhcp')" label-cols-sm="4">
        <b-form-radio-group buttons v-model="useDHCP" required>
          <b-form-radio :value="true">{{ $t('enabled') }}</b-form-radio>
          <b-form-radio :value="false">{{ $t('disabled') }}</b-form-radio>
        </b-form-radio-group>
      </b-form-group>
      <b-form-group :label="$t('ipAddress')" label-cols-sm="4" v-if="!useDHCP">
        <b-form-input
          type="text"
          v-model="$v.localIP.$model"
          trim
          :state="validateState('localIP')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('netmask')" label-cols-sm="4" v-if="!useDHCP">
        <b-form-input
          type="text"
          v-model="$v.netmask.$model"
          trim
          :state="validateState('netmask')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('gateway')" label-cols-sm="4" v-if="!useDHCP">
        <b-form-input
          type="text"
          v-model="$v.gateway.$model"
          trim
          :state="validateState('gateway')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('dns1')" label-cols-sm="4" v-if="!useDHCP">
        <b-form-input type="text" v-model="$v.dns1.$model" trim :state="validateState('dns1')"></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('dns2')" label-cols-sm="4" v-if="!useDHCP">
        <b-form-input type="text" v-model="$v.dns2.$model" trim :state="validateState('dns2')"></b-form-input>
      </b-form-group>
      <hr />
      <b-form-group :label="$t('timesource')" label-cols-sm="4">
        <b-form-radio-group buttons v-model="timesource" required>
          <b-form-radio :value="0">{{ $t('ntp') }}</b-form-radio>
          <b-form-radio :value="1">{{ $t('dcf') }}</b-form-radio>
          <b-form-radio :value="2">{{ $t('gps') }}</b-form-radio>
        </b-form-radio-group>
      </b-form-group>
      <b-form-group :label="$t('ntpServer')" label-cols-sm="4" v-if="isNtpActivated">
        <b-form-input
          type="text"
          v-model="$v.ntpServer.$model"
          trim
          :state="validateState('ntpServer')"
        ></b-form-input>
      </b-form-group>
      <b-form-group :label="$t('dcfOffset')" label-cols-sm="4" v-if="isDcfActivated">
        <b-input-group append="µs">
          <b-form-input
            type="number"
            v-model.number="$v.dcfOffset.$model"
            min="0"
            :state="validateState('dcfOffset')"
          ></b-form-input>
        </b-input-group>
      </b-form-group>
      <b-form-group :label="$t('gpsBaudrate')" label-cols-sm="4" v-if="isGpsActivated">
        <b-form-select v-model.number="gpsBaudrate">
          <b-form-select-option :value="4800">4800</b-form-select-option>
          <b-form-select-option :value="9600">9600</b-form-select-option>
          <b-form-select-option :value="19200">19200</b-form-select-option>
          <b-form-select-option :value="38400">38400</b-form-select-option>
          <b-form-select-option :value="57600">57600</b-form-select-option>
          <b-form-select-option :value="115200">115200</b-form-select-option>
        </b-form-select>
      </b-form-group>
      <hr />
      <b-form-group :label="$t('ledBrightness')" label-cols-sm="4">
        <b-input-group append="%">
          <b-form-select v-model.number="ledBrightness">
            <b-form-select-option :value="0">0</b-form-select-option>
            <b-form-select-option :value="5">5</b-form-select-option>
            <b-form-select-option :value="10">10</b-form-select-option>
            <b-form-select-option :value="25">25</b-form-select-option>
            <b-form-select-option :value="50">50</b-form-select-option>
            <b-form-select-option :value="75">75</b-form-select-option>
            <b-form-select-option :value="100">100</b-form-select-option>
          </b-form-select>
        </b-input-group>
      </b-form-group>

      <b-alert
        variant="success"
        :show="showSuccess"
        dismissible
        fade
        @dismissed="showSuccess=null"
      >{{ $t("saveSuccess") }}</b-alert>
      <b-alert
        variant="danger"
        :show="showError"
        dismissible
        fade
        @dismissed="showError=null"
      >{{ $t("saveError") }}</b-alert>

      <b-form-group label-cols-sm="9">
        <b-button
          variant="primary"
          block
          @click="saveSettingsClick"
          :disabled="$v.$anyError"
        >{{ $t('save') }}</b-button>
      </b-form-group>
    </b-form>
  </b-card>
</template>

<script>
import Vue from "vue";

import Vuelidate from "vuelidate";
Vue.use(Vuelidate);
import {
  required,
  requiredIf,
  requiredUnless,
  minLength,
  maxLength,
  numeric,
  ipAddress,
  sameAs,
  helpers
} from "vuelidate/lib/validators";

const hostname = helpers.regex('hostname', /^[a-zA-Z0-9_-]{1,63}$/)
const domainname = helpers.regex('domainname', /^([a-zA-Z0-9_-]{1,63}\.)*[a-zA-Z0-9_-]{1,63}$/)

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

import { FormRadioPlugin } from "bootstrap-vue/esm/components/form-radio";
Vue.use(FormRadioPlugin);

import { FormSelectPlugin } from "bootstrap-vue/esm/components/form-select";
Vue.use(FormSelectPlugin);

import { InputGroupPlugin } from "bootstrap-vue/esm/components/input-group";
Vue.use(InputGroupPlugin);

import { ButtonPlugin } from "bootstrap-vue/esm/components/button";
Vue.use(ButtonPlugin);

import { CardPlugin } from "bootstrap-vue/esm/components/card";
Vue.use(CardPlugin);

import { AlertPlugin } from "bootstrap-vue/esm/components/alert";
Vue.use(AlertPlugin);

export default {
  name: "Settings",
  data() {
    return {
      adminPassword: "",
      adminPasswordRepeat: "",
      hostname: "",
      useDHCP: true,
      localIP: "",
      netmask: "",
      gateway: "",
      dns1: "",
      dns2: "",
      timesource: 0,
      dcfOffset: 0,
      gpsBaudrate: 9600,
      ntpServer: "",
      ledBrightness: 100,

      showSuccess: null,
      showError: null
    };
  },
  created() {
    this.hostname = this.$store.state.settings.hostname;
    this.useDHCP = this.$store.state.settings.useDHCP;
    this.localIP = this.$store.state.settings.localIP;
    this.netmask = this.$store.state.settings.netmask;
    this.gateway = this.$store.state.settings.gateway;
    this.dns1 = this.$store.state.settings.dns1;
    this.dns2 = this.$store.state.settings.dns2;
    this.timesource = this.$store.state.settings.timesource;
    this.gpsBaudrate = this.$store.state.settings.gpsBaudrate;
    this.ntpServer = this.$store.state.settings.ntpServer;
    this.ledBrightness = this.$store.state.settings.ledBrightness;

    this.unwatch = this.$store.watch(
      (state, getters) => {
        return state.settings;
      },
      (value, oldValue) => {
        this.hostname = value.hostname;
        this.useDHCP = value.useDHCP;
        this.localIP = value.localIP;
        this.netmask = value.netmask;
        this.gateway = value.gateway;
        this.dns1 = value.dns1;
        this.dns2 = value.dns2;
        this.timesource = value.timesource;
        this.gpsBaudrate = value.gpsBaudrate;
        this.ntpServer = value.ntpServer;
        this.ledBrightness = value.ledBrightness;
      },
      { deep: true }
    );
  },
  beforeDestroy() {
    this.unwatch();
  },
  computed: {
    isNtpActivated: function() {
      return this.timesource == 0;
    },
    isDcfActivated: function() {
      return this.timesource == 1;
    },
    isGpsActivated: function() {
      return this.timesource == 2;
    }
  },
  validations: {
    adminPassword: {
      minLength: minLength(5),
      maxLength: maxLength(32)
    },
    adminPasswordRepeat: {
      sameAsPassword: sameAs("adminPassword")
    },
    hostname: {
      required,
      hostname,
      maxLength: maxLength(32)
    },
    localIP: {
      required: requiredUnless("useDHCP"),
      ipAddress
    },
    netmask: {
      required: requiredUnless("useDHCP"),
      ipAddress
    },
    gateway: {
      required: requiredUnless("useDHCP"),
      ipAddress
    },
    dns1: {
      required: requiredUnless("useDHCP"),
      ipAddress
    },
    dns2: {
      ipAddress
    },
    ntpServer: {
      required: requiredIf("isNtpActived"),
      domainname,
      maxLength: maxLength(64)
    },
    dcfOffset: {
      required: requiredIf("isDcfActived"),
      numeric
    }
  },
  mounted() {
    this.$store.dispatch("settings/load");
  },
  methods: {
    validateState(name) {
      const { $dirty, $error } = this.$v[name];
      return $dirty && $error ? false : null;
    },
    saveSettingsClick(event) {
      var self = this;

      self.$v.$touch();
      if (self.$v.$anyError) return;

      this.showError = false;
      this.showSuccess = false;

      this.$store
        .dispatch("settings/save", {
          adminPassword: self.adminPassword,
          hostname: self.hostname,
          useDHCP: self.useDHCP,
          localIP: self.localIP,
          netmask: self.netmask,
          gateway: self.gateway,
          dns1: self.dns1,
          dns2: self.dns2,
          timesource: self.timesource,
          dcfOffset: self.dcfOffset,
          gpsBaudrate: self.gpsBaudrate,
          ntpServer: self.ntpServer,
          ledBrightness: self.ledBrightness
        })
        .then(
          () => {
            self.showSuccess = true;
          },
          () => {
            self.showError = true;
          }
        );
    }
  },
  i18n: {
    locale: navigator.language,
    fallbackLocale: "en",
    messages: {
      de: {
        title: "Einstellungen",
        changePassword: "Passwort ändern",
        repeatPassword: "Passwort wiederholen",
        hostname: "Hostname",
        dhcp: "DHCP",
        enabled: "Aktiv",
        disabled: "Deaktiviert",
        ipAddress: "IP Adresse",
        netmask: "Netzmaske",
        gateway: "Gateway",
        dns1: "Primärer DNS Server",
        dns2: "Sekundärer DNS Server",
        timesource: "Zeitquelle",
        ntp: "NTP",
        dcf: "DCF",
        gps: "GPS",
        ntpServer: "NTP Server",
        dcfOffset: "DCF Versatz",
        gpsBaudrate: "GPS Baudrate",
        ledBrightness: "LED Helligkeit",
        save: "Speichern",
        saveSuccess:
          "Einstellungen wurden erfolgreich gespeichert. Bitte starten Sie das System neu um sie zu übernehmen.",
        saveError:
          "Beim Speichern der Einstellungen ist ein Fehler aufgetreten."
      },
      en: {
        title: "Settings",
        changePassword: "Change Password",
        repeatPassword: "Repeat Password",
        hostname: "Hostname",
        dhcp: "DHCP",
        enabled: "Enabled",
        disabled: "Disabled",
        ipAddress: "IP address",
        netmask: "Netmask",
        gateway: "Gateway",
        dns1: "Primary DNS",
        dns2: "Secondary DNS",
        timesource: "Timesource",
        ntp: "NTP",
        dcf: "DCF",
        gps: "GPS",
        ntpServer: "NTP Server",
        dcfOffset: "DCF Offset",
        gpsBaudrate: "GPS Baudrate",
        ledBrightness: "LED brightness",
        save: "Save",
        saveSuccess:
          "Settings were successfully saved. Please restart to take them effect.",
        saveError: "An error occured while saving the settings."
      }
    }
  }
};
</script>

<style lang="css">
</style>