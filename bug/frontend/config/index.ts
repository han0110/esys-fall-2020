import getConfig from 'next/config'
import mergeDeepRight from 'ramda/src/mergeDeepRight'
import publicRuntime from './publicRuntime'

type RuntimeConfig = typeof publicRuntime & {
  dev: Boolean
  prod: Boolean
}

const { publicRuntimeConfig } = getConfig()

const config = mergeDeepRight(publicRuntimeConfig, {
  dev: process.env.NODE_ENV !== 'production',
  prod: process.env.NODE_ENV === 'production',
})

export default config as RuntimeConfig
