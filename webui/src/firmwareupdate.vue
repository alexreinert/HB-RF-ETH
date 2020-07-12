<template>
  <b-card
    :header="$t('title')"
    header-tag="h6"
    header-bg-variant="secondary"
    header-text-variant="white"
    class="mb-3"
  >
    <b-form @submit.stop.prevent>
      <b-form-group :label="$t('installedVersion')" label-cols-sm="4">
        <b-form-input type="text" v-model="this.$store.state.sysInfo.currentVersion" disabled></b-form-input>
      </b-form-group>
      <b-alert
        variant="warning"
        :show="this.$store.state.sysInfo.currentVersion < this.$store.state.sysInfo.latestVersion && this.$store.state.sysInfo.latestVersion != 'n/a'"
      >
        <i18n path="updateAvailable">
          <template #latestVersion>
            <span>{{ latestVersion }}</span>
          </template>
          <template #github>
            <a
              href="https://github.com/alexreinert/HB-RF-ETH/releases"
              class="alert-link"
              target="_new"
            >Github</a>
          </template>
        </i18n>
      </b-alert>
      <b-form-group :label="$t('updateFile')" label-cols-sm="4">
        <b-form-file
          v-model="file"
          accept=".bin"
          :placeholder="$t('noFileChosen')"
          :browse-text="$t('browse')"
        ></b-form-file>
      </b-form-group>
      <b-progress
        :value="this.$store.state.firmwareUpdate.progress"
        max="100"
        class="mb-3"
        v-if="this.$store.state.firmwareUpdate.progress>0"
        animated
      ></b-progress>
      <b-alert
        variant="success"
        :show="showSuccess"
        dismissible
        fade
        @dismissed="showSuccess=null"
      >{{ $t("uploadSuccess") }}</b-alert>
      <b-alert
        variant="danger"
        :show="showError"
        dismissible
        fade
        @dismissed="showError=null"
      >{{ $t("uploadError") }}</b-alert>
      <b-form-group label-cols-sm="9">
        <b-button
          variant="primary"
          block
          :disabled="file==null||this.$store.state.firmwareUpdate.progress>0"
          @click="firmwareUpdateClick"
        >{{ $t('upload') }}</b-button>
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

import { FormFilePlugin } from "bootstrap-vue/esm/components/form-file";
Vue.use(FormFilePlugin);

import { ProgressPlugin } from "bootstrap-vue/esm/components/progress";
Vue.use(ProgressPlugin);

import { CardPlugin } from "bootstrap-vue/esm/components/card";
Vue.use(CardPlugin);

export default {
  name: "FirmwareUpdate",
  data() {
    return {
      file: null,
      showError: false,
      showSuccess: false
    };
  },
  computed: {
    latestVersion() {
      return this.$store.state.sysInfo.latestVersion;
    }
  },
  methods: {
    firmwareUpdateClick(event) {
      var self = this;
      this.showError = null;
      this.showSuccess = null;
      this.$store.dispatch("firmwareUpdate/update", this.file).then(
        () => {
          self.showSuccess = true;
          self.file = null;
        },
        () => {
          self.showError = true;
        }
      );
    }
  },
  mounted() {
    this.$store.dispatch("sysInfo/update");
  },
  i18n: {
    locale: navigator.language,
    fallbackLocale: "en",
    messages: {
      de: {
        title: "Firmware",
        installedVersion: "Installierte Version",
        updateAvailable:
          "Ein Update auf Version {latestVersion} ist auf {github} verfügbar.",
        updateFile: "Firmware Datei",
        noFileChosen: "Keine Datei ausgewählt",
        browse: "Datei auswählen",
        upload: "Hochladen",
        uploadSuccess:
          "Die Firmware wurde erfolgreich hochgeladen. Bitte starten Sie das System neu um sie zu aktivieren.",
        uploadError: "Es ist ein Fehler aufgetreten."
      },
      en: {
        title: "Firmware",
        installedVersion: "Installed version",
        updateAvailable:
          "An update to version {latestVersion} is available at {github}.",
        updateFile: "Firmware file",
        noFileChosen: "No file chosen",
        browse: "Browse",
        upload: "Upload",
        uploadSuccess:
          "Firmware Update successfully uploaded. Please restart to activate.",
        uploadError: "An error occured."
      }
    }
  }
};
</script>

<style lang="css">
</style>